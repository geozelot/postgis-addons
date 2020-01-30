-- DROP FUNCTION pgr_ksp(TEXT, BIGINT[], INTEGER, BOOLEAN, BOOLEAN);

CREATE OR REPLACE FUNCTION pgr_ksp(

    IN edges_sql TEXT,
    IN vids BIGINT[],
    IN k INTEGER,
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
	
      SELECT (ROW_NUMBER() OVER(ORDER BY ksp.path_id, ksp.itr, ksp.path_seq))::INTEGER AS seq,
	     (DENSE_RANK() OVER(ORDER BY ksp.path_id))::INTEGER AS path_id,
	     (ROW_NUMBER() OVER(PARTITION BY ksp.path_id ORDER BY ksp.path_id))::INTEGER AS path_seq,
	      ksp.node AS node,
	      ksp.edge AS edge,
	      ksp.COST AS COST,
	      CASE
		WHEN ksp.edge = -1
		  THEN (SUM(ksp.cost) OVER(PARTITION BY ksp.path_id ORDER BY ksp.itr, ksp.path_seq))::DOUBLE PRECISION
		  ELSE (SUM(ksp.cost) OVER(PARTITION BY ksp.path_id ORDER BY ksp.itr, ksp.path_seq) - 1)::DOUBLE PRECISION
	      END AS agg_cost
      FROM (
        SELECT (_pgr_ksp(
                  edges_sql::TEXT,
                  vids[n],
                  vids[n+1],
                  k,
                  directed,
                  heap_paths
              	)).*,
                n AS itr
        FROM generate_series(1, len) AS n
      ) AS ksp
      WHERE NOT (ksp.edge = -1 AND itr < len);

  END

$BODY$

LANGUAGE plpgsql VOLATILE
COST 100
ROWS 1000
