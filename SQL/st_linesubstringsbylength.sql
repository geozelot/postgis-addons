-- DROP FUNCTION IF EXISTS ST_LineSubstringsByLength(GEOMETRY, FLOAT8, BOOLEAN);

CREATE OR REPLACE FUNCTION ST_LineSubstringsByLength(

  geom       GEOMETRY(LINESTRING),
  length     FLOAT8,
  use_meter  BOOLEAN DEFAULT TRUE

) RETURNS SETOF geometry_dump AS

  $BODY$

    DECLARE

      len_frac  FLOAT8;
      s_frac    FLOAT8;
      e_frac    FLOAT8;

    BEGIN

      IF ($3)
        THEN  len_frac := $2 / ST_Length(geom::GEOGRAPHY);
        ELSE  len_frac := $2 / ST_Length(geom);
      END IF;

      FOR n IN 0..CEIL(1.0 / len_frac)
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

  LANGUAGE plpgsqlVOLATILE
  COST 100
  ROWS 1000;