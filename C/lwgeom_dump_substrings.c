/*
-- Availability: CUSTOM
CREATE OR REPLACE FUNCTION ST_LineSubstringsByLength(geometry, float8)
  RETURNS SETOF geometry_dump AS
  $$ SELECT @extschema@._ST_DumpSubstrings($1, $2 / @extschema@.ST_Length($1));
  $$
  LANGUAGE 'sql' IMMUTABLE STRICT PARALLEL SAFE;

-- Availability: CUSTOM
CREATE OR REPLACE FUNCTION ST_LineSubstringsByLength(geography, float8)
  RETURNS SETOF geometry_dump AS
  $$ SELECT @extschema@._ST_DumpSubstrings($1::geometry, $2 / @extschema@.ST_Length($1));
  $$
  LANGUAGE 'sql' IMMUTABLE STRICT PARALLEL SAFE;

-- Availability: CUSTOM
CREATE OR REPLACE FUNCTION ST_LineSubstringsBySegment(geometry, int)
  RETURNS SETOF geometry_dump AS
  $$ SELECT @extschema@._ST_DumpSubstrings($1, 1.0 / $2::float8);
  $$
  LANGUAGE 'sql' IMMUTABLE STRICT PARALLEL SAFE;

-----------------------------------------------------------------------
-- _ST_DumpSubstrings()
-----------------------------------------------------------------------
-- This function cuts a geometries line component into substrings of
-- defined fractional length; used as base method for
-- ST_LineSubstringsByLength und ST_LineSubstringsBySegment
-- Availability: CUSTOM
CREATE OR REPLACE FUNCTION _ST_DumpSubstrings(geometry, float8)
        RETURNS SETOF geometry_dump
  AS 'MODULE_PATHNAME', 'LWGEOM_dump_substrings'
  LANGUAGE 'c' IMMUTABLE STRICT _PARALLEL
  COST 100;
*/

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

/* Substring Suite for PostGIS; creating and dumping
 * substrings of defined length or count of line components.
 * By geozelot, copyright disclaimed,
 * this entire file is in the public domain
 */

Datum LWGEOM_dump_substrings(PG_FUNCTION_ARGS);

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

PG_FUNCTION_INFO_V1(LWGEOM_dump_substrings);
Datum LWGEOM_dump_substrings(PG_FUNCTION_ARGS) {
  FuncCallContext *funcctx;
  MemoryContext oldcontext, newcontext;

  GSERIALIZED *pglwgeom;
  LWCOLLECTION *lwcoll;
  LWGEOM *lwgeom;
  double segfrac = PG_GETARG_FLOAT8(1);
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

    /* get a local copy of what we're doing a dump points on */
    pglwgeom = PG_GETARG_GSERIALIZED_P_COPY(0);
    lwgeom = lwgeom_from_gserialized(pglwgeom);

    /* return early if nothing to do */
    if (!lwgeom || lwgeom_is_empty(lwgeom) || segfrac == 0.0) {
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

    if (!lwgeom_is_collection(lwgeom)) {

      LWLINE  *line;
      LWCIRCSTRING *circ;
      LWPOLY  *poly;
      //LWTRIANGLE  *tri;
      LWLINE *lwsegment = NULL;
      POINTARRAY *ipa, *opa;

      switch(lwgeom->type) {
        /*case TRIANGLETYPE:
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
          break;*/
        case POLYGONTYPE:
          poly = lwgeom_as_lwpoly(lwgeom);
          if ((segfrac*(state->pt)) >= 1.0) {
            state->pt = 0;
            state->ring++;
            state->pathlen--;
          }
          if (state-> pt == 0 && state->ring < poly->nrings) {
            state->path[state->pathlen] = Int32GetDatum(state->ring+1);
            state->pathlen++;
          }
          if (state->ring == poly->nrings) {
          } else {
            ipa = poly->rings[state->ring];
            opa = ptarray_substring(ipa, segfrac*state->pt, segfrac*(state->pt+1), 0);
            lwsegment = (LWLINE *)lwline_construct(poly->srid, NULL, opa);
          }
          break;
        case LINETYPE:
          if ((segfrac*(state->pt)) < 1.0) {
            line = lwgeom_as_lwline(lwgeom);
            ipa = line->points;
            opa = ptarray_substring(ipa, segfrac*state->pt, segfrac*(state->pt+1), 0);
            lwsegment = (LWLINE *)lwline_construct(line->srid, NULL, opa);
          }
          break;
        case CIRCSTRINGTYPE:
          if ((segfrac*(state->pt)) < 1.0) {
            circ = lwgeom_as_lwcircstring(lwgeom);
            ipa = circ->points;
            opa = ptarray_substring(ipa, segfrac*state->pt, segfrac*(state->pt+1), 0);
            lwsegment = (LWLINE *)lwline_construct(circ->srid, NULL, opa);
          }
          break;
        default:
          ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
            errmsg("Invalid Geometry type %d passed to _ST_DumpSubstrings()", lwgeom->type)));
      }

      

      if (!lwsegment) {
        if (--state->stacklen == 0) SRF_RETURN_DONE(funcctx);
        state->pathlen--;
        continue;
      } else {
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

    if (--state->stacklen == 0) SRF_RETURN_DONE(funcctx);
    state->pathlen--;
    state->stack[state->stacklen-1].idx++;

  }

}