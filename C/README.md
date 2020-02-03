### C function core addons

* #### function set `LWGEOM_dump_segments`: <br>
  ##### `GEOMETRY_DUMP ST_DumpSegments(geom GEOMETRY)`<br>
  Dumps the linear component(s) of the given geometry `geom` into the minimal (two-vertice) segments.<br>
  Returns a `GEOMETRY_DUMP` having a `path INT[]` and `geometry GEOMETRY` member.
  
* #### function set `LWGEOM_dump_substrings`: <br>
  ##### `GEOMETRY_DUMP ST_LineSubstringsByLength(geom GEOMETRY, seg_len FLOAT8)` <br>
  ##### `GEOMETRY_DUMP ST_LineSubstringsByLength(geog GEOGRAPHY, seg_len FLOAT8)`
  Creates substrings of the linear component of the given geometry `geom` having a length of `seg_len` each;
  segments will be created starting with the `ST_StartPoint`, and last segment may be shorter than `seg_len`.<br>
  Returns a `GEOMETRY_DUMP` having a `path INT[]` and `geometry GEOMETRY` member.
  <br>
  <br>
  ##### `GEOMETRY_DUMP ST_LineSubstringsBySegment(geom GEOMETRY, seg_cnt INT)`
  Creates `seg_cnt` equal length substrings of the linear component of the given geometry `geom`.<br>
  Returns a `GEOMETRY_DUMP` having a `path INT[]` and `geometry GEOMETRY` member.
  <br>
  <br>
  ##### `SETOF GEOMETRY_DUMP _ST_DumpSegments(geom GEOMETRY, len_frac FLOAT)`
  Utility C function to create segments from the linear compinent of `geom` using a fraction (`len_frac`) in sequence. Get's called by `ST_LineSubstringsByLength` & `ST_LineSubstringBySegments`.
