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
* #### function set `st_linesubstringsbylength`: <br>
  ##### `SETOF GEOMETRY_DUMP ST_LineSubstringsByLength(geom GEOMETRY(LINESTRING), seg_len FLOAT8)`<br>
  ##### `SETOF GEOMETRY_DUMP ST_LineSubstringsByLength(geom GEOGRAPHY(LINESTRING), seg_len FLOAT8)`<br>
  Creates substrings of the given LineString `geom` having a length of `seg_len` each;
  segments will be created starting with the `ST_StartPoint`, and last segment may be shorter than `seg_len`.<br>
  Returns a `GEOMETRY_DUMP` having a `path INT[]` and `geometry GEOMETRY` member.
  <br>
  <br>
  ##### `SETOF GEOMETRY_DUMP _ST_DumpSegments(geom GEOMETRY(LINSTRING), len_frac FLOAT)`
  Utility function wrapping around `ST_LineSubstring` to create segments from `geom` using a fraction (`len_frac`) in sequence. Get's called by `ST_LineSubstringsByLength`.
