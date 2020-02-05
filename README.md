## pg-addons

### Custom functions for PostGIS & pgRouting

* #### Folder `SQL` contains SQL based addon functions
  * `*.sql` files can be added by executing the file or the SQL as DDL
  * may need _pgRouting_ installed
  
* #### Folder `C` contains C based core addon functions
  All function listed here are implemented in my [custom PostGIS source](https://github.com/geozelot/pg-custom), either build from there, or use the original source to build:
  * get latest tarball from https://postgis.net/source/
  * copy `*.c` files to
  <br>`postgis-<version>/postgis/`
  * add function names (as per `Datum <funcname>` definition in source file) to
  <br>`postgis-<version>/postgis/Makefile.in` (after line 90)
  * add SQL function definitions (in same name `*.sql` file) to
  <br>`postgis-<version>/postgis/postgis.sql.in` (look for appropriate category in comments)
  * run
    ```
    ./configure
    sudo make && sudo make install
    ```
  * restart PostgreSQL server
  
