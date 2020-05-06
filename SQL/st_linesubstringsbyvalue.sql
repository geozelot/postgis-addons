-- DROP FUNCTION IF EXISTS ST_LineSubstringsByLength(GEOMETRY, FLOAT8);
-- DROP FUNCTION IF EXISTS ST_LineSubstringsByLength(GEOGRAPHY, FLOAT8);

-- DROP FUNCTION IF EXISTS ST_LineSubstringsBySegment(GEOMETRY, INTEGER);
-- DROP FUNCTION IF EXISTS ST_LineSubstringsBySegment(GEOGRAPHY, INTEGER);

-- DROP FUNCTION IF EXISTS _ST_DumpSubstrings(GEOMETRY, FLOAT8);


CREATE OR REPLACE FUNCTION _ST_DumpSubstrings(

  geom       GEOMETRY(LINESTRING),
  len_frac   FLOAT8

) RETURNS SETOF geometry_dump AS

  $BODY$

    DECLARE

      s_frac    FLOAT8;
      e_frac    FLOAT8;

    BEGIN

      FOR n IN 0..CEIL(ST_Length(geom) / len_frac)
        LOOP
          s_frac := len_frac * n;
          IF (s_frac >= 1.0)
            THEN
              EXIT;
          END IF;
          e_frac := len_frac * (n + 1);
          IF (e_frac > 1.0)
            THEN
              e_frac := 1.0;
          END IF;
          RETURN NEXT (ARRAY[n + 1], ST_LineSubstring($1, s_frac, e_frac));
        END LOOP;

      RETURN;

    END;

  $BODY$

  LANGUAGE plpgsql VOLATILE
  COST 100
  ROWS 1000;


CREATE OR REPLACE FUNCTION ST_LineSubstringsByLength(geom GEOMETRY, seg_len FLOAT8)
  RETURNS SETOF geometry_dump AS
  $$ SELECT _ST_DumpSubstrings($1, $2 / ST_Length($1));
  $$
  LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ST_LineSubstringsByLength(geog GEOGRAPHY, seg_len FLOAT8)
  RETURNS SETOF geometry_dump AS
  $$ SELECT _ST_DumpSubstrings($1::geometry, $2 / ST_Length($1));
  $$
  LANGUAGE 'sql' IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION ST_LineSubstringsBySegment(geom GEOMETRY, seg_cnt INTEGER)
  RETURNS SETOF geometry_dump AS
  $$ SELECT _ST_DumpSubstrings($1, 1.0 / $2::FLOAT8);
  $$
  LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ST_LineSubstringsBySegment(geom GEOGRAPHY, seg_cnt INTEGER)
  RETURNS SETOF geometry_dump AS
  $$ SELECT _ST_DumpSubstrings($1::geometry, 1.0 / $2::FLOAT8);
  $$
  LANGUAGE 'sql' IMMUTABLE STRICT;
