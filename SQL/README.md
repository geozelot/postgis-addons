### SQL function addons

* #### function `pgr_ksp_vids`: <br>
  ##### `SETOF RECORD pgr_KSP(edges_sql TEXT, vids BIGINT[], k INT, directed BOOLEAN, heap_path BOOLEAN)`<br>
  A _**via point** k shortest path_ wrapper around the _pgRouting_ `pgr_KSP` core function.<br>
  Signature is identical, **exception is the `vids` parameter**, accepting an array of points to be routed along in sequence;<br>
  will effectively call the core `pgr_KSP` function in sequence for any two `vids` in the array, and accumulates the results.
  Returns
    ```
    SETOF RECORD(
      seq       INT,
      path_id   INT,
      path_seq  INT,
      path_id   BIGINT,
      node      BIGINT,
      edge      BIGINT,
      cost      FLOAT8,
      agg_cost  FLOAT8
    )
    ```
* #### function `st_linesubstringsbylength`: <br>
  ##### `SETOF GEOMETRY_DUMP ST_LineSubstringsByLength(geom GEOMETRY(LINESTRING), length FLOAT8, use_meter BOOLEAN DEFAULT TRUE)`<br>
  Convenience PL/pgSQL function to create substrings of a given LineString based on `length` parameter. Will cast to `GEOGRAPHY` by default (@param `use_meter`) to work with meter as units (**requires a geodetic reference system for the `geom` input!**); if not desired, set to `FALSE` to use CRS units.
  Returns a PostGIS `GEOMETRY_DUMP` having a `path BIGINT[]` sequence member and resective a `geom GEOMETRY`. 
