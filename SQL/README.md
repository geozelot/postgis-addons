### SQL function addons

* #### function `pgr_ksp_vids`: <br>
  ##### `SETOF RECORD pgr_KSP(edges_sql TEXT, vids BIGINT[], k INT, directed BOOLEAN, heap_path BOOLEAN)`<br>
  A _**via point** k shortest path_ wrapper around the _pgRouting_ `pgr_KSP` core function.<br>
  Signature is identical, **exception is the `vids` parameter**, accepting an array of points to be routed along in sequence;<br>
  will effectively call the core `pgr_KSP` function in sequence for any two `vids` in the array, and accumulates the results.<br>
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
  Creates substrings of the given LineString `geom` having a length of `length` each;
  segments will be created starting with the `ST_StartPoint`, and last segment may be shorter than `length`.<br>
  Will cast to `GEOGRAPHY` by default (@param `use_meter`) to work with meter as units of segment length (**requires a geodetic reference system for the `geom` input!**); if not desired or available, set to `FALSE` to use CRS units.<br>
  Returns a `GEOMETRY_DUMP` having a `path INT[]` and `geometry GEOMETRY` member.
