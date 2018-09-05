-- DROP FUNCTION pgr_ksp(TEXT, BIGINT[], BIGINT, BOOLEAN, BOOLEAN);

CREATE OR REPLACE FUNCTION pgr_ksp(

    IN edges_sql TEXT,
    IN vids BIGINT[],
    IN k BIGINT,
    IN directed boolean DEFAULT true,
    IN heap_paths boolean DEFAULT false,
    OUT seq integer,
    OUT path_id integer,
    OUT path_seq integer,
    OUT node bigint,
    OUT edge bigint,
    OUT cost double precision,
    OUT agg_cost double precision

  ) RETURNS SETOF record AS

$BODY$

  DECLARE

	len INTEGER := array_length(vids, 1) - 1;


  BEGIN

	RETURN QUERY
	
		SELECT (ROW_NUMBER() OVER())::INTEGER AS seq,
		       (DENSE_RANK() OVER(PARTITION BY ksp.path_id ORDER BY ksp.path_id))::INTEGER AS path_id,
		       (ROW_NUMBER() OVER(PARTITION BY ksp.path_id  ORDER BY ksp.path_id))::INTEGER AS path_seq,
		       ksp.node AS node,
		       ksp.edge AS edge,,
		       ksp.COST AS COST,
		       (ksp.cost + lag(ksp.COST) OVER(PARTITION BY ksp.path_id))::DOUBLE PRECISION AS agg_cost
		FROM _pgr_ksp(
				edges_sql::TEXT,
				vids[n],
				vids[n+1],
				cost,
				reverse_cost
		     ) AS ksp,
		     generate_series(1, len) AS n;

  END

$BODY$

LANGUAGE plpgsql VOLATILE
COST 100
ROWS 1000	