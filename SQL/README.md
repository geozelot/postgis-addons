### SQL function addons

* #### function set `pgr_ksp_vids`: <br>
  ##### `SETOF RECORD pgr_KSP(edges_sql TEXT, vids BIGINT[], k int, directed bool, heap_path bool)`<br>
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
