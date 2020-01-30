-----------------------------------------------------------------------
-- ST_DumpSegments()
-----------------------------------------------------------------------
-- This function mimicks that of ST_Dump for collections, but this function
-- returns a path and all (two-vertice) line segments that make up a particular geometry.
-- Availability: CUSTOM
CREATE OR REPLACE FUNCTION ST_DumpSegments(geometry)
        RETURNS SETOF geometry_dump
  AS 'MODULE_PATHNAME', 'LWGEOM_dump_segments'
  LANGUAGE 'c' IMMUTABLE STRICT PARALLEL SAFE
  COST 100;