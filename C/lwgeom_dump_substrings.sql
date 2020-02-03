-- Availability: CUSTOM
CREATE OR REPLACE FUNCTION ST_LineSubstringsByLength(geom geometry, seg_len float8)
  RETURNS SETOF geometry_dump AS
  $$ SELECT @extschema@._ST_DumpSubstrings($1, $2 / @extschema@.ST_Length($1));
  $$
  LANGUAGE 'sql' IMMUTABLE STRICT _PARALLEL;

-- Availability: CUSTOM
CREATE OR REPLACE FUNCTION ST_LineSubstringsByLength(geom geography, seg_len float8)
  RETURNS SETOF geometry_dump AS
  $$ SELECT @extschema@._ST_DumpSubstrings($1::geometry, $2 / @extschema@.ST_Length($1));
  $$
  LANGUAGE 'sql' IMMUTABLE STRICT _PARALLEL;

-- Availability: CUSTOM
CREATE OR REPLACE FUNCTION ST_LineSubstringsBySegment(geom geometry, seg_cnt int)
  RETURNS SETOF geometry_dump AS
  $$ SELECT @extschema@._ST_DumpSubstrings($1, 1.0 / $2::float8);
  $$
  LANGUAGE 'sql' IMMUTABLE STRICT _PARALLEL;

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
