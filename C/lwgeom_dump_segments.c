/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * ^copyright^
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/array.h"
#include "utils/geo_decls.h"
#include "utils/lsyscache.h"
#include "catalog/pg_type.h"
#include "funcapi.h"

#include "../postgis_config.h"
#include "lwgeom_pg.h"

#include "access/htup_details.h"


#include "liblwgeom.h"

/* ST_DumpSegments for PostGIS; dumping
 * two vertice segments of any line component.
 * By geozelot, copyright disclaimed,
 * this entire file is in the public domain
 */

Datum LWGEOM_dump_segments(PG_FUNCTION_ARGS);

struct dumpnode {
  LWGEOM *geom;
  uint32_t idx; /* which member geom we're working on */
} ;

/* 32 is the max depth for st_dump, so it seems reasonable
 * to use the same here
 */
#define MAXDEPTH 32
struct dumpstate {
  LWGEOM  *root;
  int stacklen; /* collections/geoms on stack */
  int pathlen; /* polygon rings and such need extra path info */
  struct  dumpnode stack[MAXDEPTH];
  Datum path[34]; /* two more than max depth, for ring and point */

  /* used to cache the type attributes for integer arrays */
  int16 typlen;
  bool  byval;
  char  align;

  uint32_t ring; /* ring of top polygon */
  uint32_t pt; /* point of top geom or current ring */
};

PG_FUNCTION_INFO_V1(LWGEOM_dump_segments);
Datum LWGEOM_dump_segments(PG_FUNCTION_ARGS) {
  FuncCallContext *funcctx;
  MemoryContext oldcontext, newcontext;

  GSERIALIZED *pglwgeom;
  LWCOLLECTION *lwcoll;
  LWGEOM *lwgeom;
  struct dumpstate *state;
  struct dumpnode *segment;

  HeapTuple tuple;
  Datum pathseg[2]; /* used to construct the composite return value */
  bool isnull[2] = {0,0}; /* needed to say neither value is null */
  Datum result; /* the actual composite return value */

  if (SRF_IS_FIRSTCALL()) {
    funcctx = SRF_FIRSTCALL_INIT();

    newcontext = funcctx->multi_call_memory_ctx;
    oldcontext = MemoryContextSwitchTo(newcontext);

    /* get a local copy of what we're doing a dump segments on */
    pglwgeom = PG_GETARG_GSERIALIZED_P_COPY(0);
    lwgeom = lwgeom_from_gserialized(pglwgeom);

    /* return early if nothing to do */
    if (!lwgeom || lwgeom_is_empty(lwgeom)) {
      MemoryContextSwitchTo(oldcontext);
      funcctx = SRF_PERCALL_SETUP();
      SRF_RETURN_DONE(funcctx);
    }

    /* Create function state */
    state = lwalloc(sizeof *state);
    state->root = lwgeom;
    state->stacklen = 0;
    state->pathlen = 0;
    state->pt = 0;
    state->ring = 0;

    funcctx->user_fctx = state;

    /*
     * Push a struct dumpnode on the state stack
     */

    state->stack[0].idx = 0;
    state->stack[0].geom = lwgeom;
    state->stacklen++;

    /*
     * get tuple description for return type
     */
    if (get_call_result_type(fcinfo, 0, &funcctx->tuple_desc) != TYPEFUNC_COMPOSITE) {
      ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
        errmsg("set-valued function called in context that cannot accept a set")));
    }

    BlessTupleDesc(funcctx->tuple_desc);

    /* get and cache data for constructing int4 arrays */
    get_typlenbyvalalign(INT4OID, &state->typlen, &state->byval, &state->align);

    MemoryContextSwitchTo(oldcontext);
  }

  /* stuff done on every call of the function */
  funcctx = SRF_PERCALL_SETUP();
  newcontext = funcctx->multi_call_memory_ctx;

  /* get state */
  state = funcctx->user_fctx;

  while (1) {
    segment = &state->stack[state->stacklen-1];
    lwgeom = segment->geom;

    /* need to return a segment from this geometry */
    if (!lwgeom_is_collection(lwgeom)) {
      /* either return a segment, or pop the stack */
      LWLINE  *line;
      LWCIRCSTRING *circ;
      LWPOLY  *poly;
      LWTRIANGLE  *tri;
      LWLINE  *lwsegment = NULL;
      POINT4D p1, p2; /*to store each extracted pair of points*/
      POINTARRAY  *tmp; /*to construct segment line geometry*/

      tmp = ptarray_construct_empty(lwgeom_has_z(lwgeom), lwgeom_has_m(lwgeom), 2);

      /*
       * net result of switch should be to set lwsegment to the
       * next segent to return, or leave at NULL if there
       * are no more point pairs in the geometry
       */
      switch(lwgeom->type) {
        case TRIANGLETYPE:
          tri = lwgeom_as_lwtriangle(lwgeom);
          if (state->pt == 0) {
            state->path[state->pathlen++] = Int32GetDatum(state->ring+1);
          }
          if (state->pt < 3) {
            getPoint4d_p(tri->points, state->pt, &p1);
            getPoint4d_p(tri->points, state->pt+1, &p2);
            ptarray_append_point(tmp, &p1, LW_TRUE);
            ptarray_append_point(tmp, &p2, LW_TRUE);
            lwsegment = (LWLINE *)lwline_construct(tri->srid, NULL, tmp);
          }
          if (state->pt >= 3) {
            state->pathlen--;
          }
          break;
        case POLYGONTYPE:
          poly = lwgeom_as_lwpoly(lwgeom);
          if (state->pt == poly->rings[state->ring]->npoints-1) {
            state->pt = 0;
            state->ring++;
            state->pathlen--;
          }
          if (state->pt == 0 && state->ring < poly->nrings) {
            /* handle new ring */
            state->path[state->pathlen] = Int32GetDatum(state->ring+1);
            state->pathlen++;
          }
                if (state->ring == poly->nrings) {
          } else {
            getPoint4d_p(poly->rings[state->ring], state->pt, &p1);
            getPoint4d_p(poly->rings[state->ring], state->pt+1, &p2);
            ptarray_append_point(tmp, &p1, LW_TRUE);
            ptarray_append_point(tmp, &p2, LW_TRUE);
            lwsegment = (LWLINE *)lwline_construct(poly->srid, NULL, tmp);
          }
          break;
        case LINETYPE:
          line = lwgeom_as_lwline(lwgeom);
          if (line->points && state->pt < line->points->npoints-1) {
            getPoint4d_p(line->points, state->pt, &p1);
            getPoint4d_p(line->points, state->pt+1, &p2);
            ptarray_append_point(tmp, &p1, LW_TRUE);
            ptarray_append_point(tmp, &p2, LW_TRUE);
            lwsegment = (LWLINE *)lwline_construct(line->srid, NULL, tmp);
          }
          break;
        case CIRCSTRINGTYPE:
          circ = lwgeom_as_lwcircstring(lwgeom);
          if (circ->points && state->pt < circ->points->npoints-1) {
            getPoint4d_p(circ->points, state->pt, &p1);
            getPoint4d_p(circ->points, state->pt+1, &p2);
            ptarray_append_point(tmp, &p1, LW_TRUE);
            ptarray_append_point(tmp, &p2, LW_TRUE);
            lwsegment = (LWLINE *)lwline_construct(circ->srid, NULL, tmp);
          }
          break;
        default:
          ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
            errmsg("Invalid Geometry type %d passed to ST_DumpSegments()", lwgeom->type)));
      }

      /*
       * At this point, lwsegment is either NULL, in which case
       * we need to pop the geometry stack and get the next
       * geometry, if any, or lwsegment is set and we construct
       * a record type with the integer array of geometry
       * indexes and the segment number, and the actual line
       * geometry itself
       */

      if (!lwsegment) {
        /* no line, so pop the geom and look for more */
        if (--state->stacklen == 0) SRF_RETURN_DONE(funcctx);
        state->pathlen--;
        continue;
      } else {
        /* write address of current geom/pt */
        state->pt++;

        state->path[state->pathlen] = Int32GetDatum(state->pt);
        pathseg[0] = PointerGetDatum(construct_array(state->path, state->pathlen+1,
            INT4OID, state->typlen, state->byval, state->align));

        pathseg[1] = PointerGetDatum(geometry_serialize((LWGEOM*)lwsegment));

        tuple = heap_form_tuple(funcctx->tuple_desc, pathseg, isnull);
        result = HeapTupleGetDatum(tuple);
        SRF_RETURN_NEXT(funcctx, result);
      }
    }

    lwcoll = (LWCOLLECTION*)segment->geom;

    /* if a collection and we have more geoms */
    if (segment->idx < lwcoll->ngeoms) {
      /* push the next geom on the path and the stack */
      lwgeom = lwcoll->geoms[segment->idx++];
      state->path[state->pathlen++] = Int32GetDatum(segment->idx);

      segment = &state->stack[state->stacklen++];
      segment->idx = 0;
      segment->geom = lwgeom;

      state->pt = 0;
      state->ring = 0;

      /* loop back to beginning, which will then check whatever segment we just pushed */
      continue;
    }

    /* no more geometries in the current collection */
    if (--state->stacklen == 0) SRF_RETURN_DONE(funcctx);
    state->pathlen--;
    state->stack[state->stacklen-1].idx++;
  }
}