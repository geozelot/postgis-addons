### C function core addons

* #### function set `LWGEOM_dump_segments`: <br>
  ##### `geometry_dump ST_DumpSegments(geom geometry)`<br>
  Dumps the linear component(s) of the given geometry `geom` into the minimal (two-vertice) segments.<br>
  Returns a `geometry_dump` having a `path INT[]` and `geometry GEOMETRY` member
  
* #### function set `LWGEOM_dump_substrings`: <br>
  ##### `geometry_dump ST_LineSubstringByLength(geom geometry, seg_len float8)` <br>
  ##### `geometry_dump ST_LineSubstringByLength(geog geography, seg_len float8)`
  Creates substrings of the linear component of the given geometry `geom` having a length of `length`;
  segments will be created starting with the `ST_StartPoint`, and last segment may be shorter than `length`.<br>
  Returns a `geometry_dump` having a `path INT[]` and `geometry GEOMETRY` member
  <br>
  <br>
  ##### `geometry_dump ST_LineSubstringBySegment(geom geometry, seg_cnt integer)`
  Creates `seg_cnt` equal length substrings of the linear component of the given geometry `geom`.<br>
  Returns a `geometry_dump` having a `path INT[]` and `geometry GEOMETRY` member
