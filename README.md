
# rrd-client-lib - A Library to Provide RRD Data

The RRD service is a system-level service that aggregates
quasi-continuous numerical data. For example, it can aggregate data like
CPU load or disk usage.  Unlike syslog, RRD aggregates the data: an
RRD plugin reports data every 5 seconds but the service aggregates older
data into statistical values (like minimum, maximum, and mean values.)

This library provides functions to register a plugin with the RRD
service and to provide data sources. All data provided by the plugin is
written periodically to a file from where it is read by the RRD daemon.

## Building

The `Makefile` builds the library and a simple test. The implementation
relies on a small JSON library that is included as a Git submodule. This
needs to be initialised:

    make parson
    make

    ./rrdtest

## Parson

The [Parson](https://github.com/kgabis/parson.git) library is included
as a Git submodule. A submodule points to a specific commit in an
external repository and does not track its master branch as this
advances. Instead, it needs to be updated explicitly.

## Documentation - Overview

The header file `librrd.h` contains the essential information to use the
library:

    typedef ...     rrd_value;
    typedef ...     RRD_SOURCE;
    typedef ...     RRD_PLUGIN;

    RRD_PLUGIN     *rrd_open(char *name, rrd_domain domain, char *path);
    int             rrd_close(RRD_PLUGIN * plugin);
    int             rrd_add_src(RRD_PLUGIN * plugin, RRD_SOURCE * source);
    int             rrd_del_src(RRD_PLUGIN * plugin, RRD_SOURCE * source);
    int             rrd_sample(RRD_PLUGIN * plugin);

A plugin reports streams of data to the RRD service. Each such
stream is represented as an `RRD_SOURCE` value. An `RRD_PLUGIN`
registers itself with `rrd_open` and un-registers with `rrd_close`. Once
registered, data sources can be added and removed with `rrd_add_src` and
`rrd_del_src`.

Data is reported to the RRD service when calling `rrd_sample`. This
samples values from all data sources and writes the data to the `path`
provided to `rrd_open`. It is the client's responsibility to call
`rrd_sample` regularly at an interval of 5 seconds.

## Sample Code

See `rrdclient.c` for a simple client that reads integers from standard
input as a data source and reports them to RRD. Here is how everything
is compiled and linked:

    gcc -std=gnu99 -g -Wall -c -o librrd.o librrd.c
    gcc -std=gnu99 -g -Wall -c -o parson/parson.o parson/parson.c
    ar rc librrd.a librrd.o parson/parson.o
    ranlib librrd.a
    gcc -std=gnu99 -g -Wall -c -o rrdclient.o rrdclient.c
    gcc -std=gnu99 -g -Wall -o rrdclient rrdclient.o librrd.a -lz

## Interface

    <<librrd.h>>=
    #include <stdint.h>
    #include "parson/parson.h"
    <<constants>>
    <<type definitions>>
    <<function declarations>>
    

All strings are in UTF8 encoding. The library implements the following
policy to manage memory: it does not free any memory that is hasn't
itself allocated. This means, if the client passes dynamically allocated
data into the library, it is the client's responsibility to de-allocate
it. 

## Open, Sample, Close

A plugin opens a file for communication and periodically calls
``rrd_sample`` to provide data. The type `RRD_PLUGIN` should be
considered private to the library.

    <<type definitions>>=
    typedef enum { RRD_LOCAL_DOMAIN = 0, RRD_INTER_DOMAIN } rrd_domain;
    
    
    <<function declarations>>=
    RRD_PLUGIN     *rrd_open(char *name, rrd_domain domain, char *path);
    int             rrd_close(RRD_PLUGIN * plugin);
    int             rrd_sample(RRD_PLUGIN * plugin);
    

The name of the plugin is descriptive, as whether it reports data
for a single machine (`RRD_LOCAL_DOMAIN`) or multiple
(`RRD_INTER_DOMAIN`).

## Data Sources

A typical client has several data sources. A data source either reports
integer or float values. Data sources can be added and removed
dynamically.

    <<type definitions>>=
    typedef enum { RRD_GAUGE = 0, RRD_ABSOLUTE, RRD_DERIVE } rrd_scale;
    typedef enum { RRD_HOST = 0, RRD_VM, RRD_SR } rrd_owner;
    typedef enum { RRD_FLOAT64 = 0, RRD_INT64 } rrd_type;
    
    typedef union {
        int64_t         int64;
        float           float64;
    } rrd_value;
    
    typedef struct rrd_source {
        char           *name;       /* name of the data source */
        char           *description;        /* for user interface */
        rrd_owner       owner;
        int             rrd_default;/* true: rrd daemon will archive */
        char           *owner_uuid; /* UUID of the owner or NULL */
        char           *rrd_units;  /* for user interface */
        rrd_scale       scale;      /* presentation of value */
        rrd_type        type;       /* type of value */
        char           *min;        /* min <= sample() <= max */
        char           *max;        /* min <= sample() <= max */
                        rrd_value(*sample) (void);  /* reads value that gets *
                                                     * reported */
    } RRD_SOURCE;
    
    
    <<typedef RRD_PLUGIN>>
    
    <<function declarations>>=
    int rrd_add_src(RRD_PLUGIN *plugin, RRD_SOURCE *source);
    int rrd_del_src(RRD_PLUGIN *plugin, RRD_SOURCE *source);
    
    
An `RRD_SOURCE` has several descriptive fields for the value it is
reporting. It includes a function (pointer) `sample` that obtains the
value being reported. A value can be either a 64-bit integer or a 64-bit
floating point value -- discriminated by `type`.

## Error Handling

Some functions return an error code.

    <<constants>>=
    #define RRD_MAX_SOURCES         128
    
    #define RRD_OK                  0
    #define RRD_TOO_MANY_SOURCES    1
    #define RRD_NO_SOUCH_SOURCE     2
    #define RRD_FILE_ERROR          3
    #define RRD_ERROR               4
    


## Private Types

    <<constants>>=
    #define RRD_MAX_SOURCES           128
    
    
    <<typedef RRD_PLUGIN>>=
    typedef struct rrd_plugin {
        char           *name;       /* name of the plugin */
        int             file;       /* where we report data */
        rrd_domain      domain;     /* domain of this plugin */
        RRD_SOURCE     *sources[RRD_MAX_SOURCES];
        uint32_t        n;          /* number of used slots */
        JSON_Value     *meta;       /* meta data for the plugin */
        char           *buf;        /* buffer where we keep protocol data */
        size_t          buf_size;   /* size of the buffer */
    } RRD_PLUGIN;
    

## Design

The library updates the file provided to `rrd_open` when `rrd_sample` is
called. The file format is combination of a binary header followed by a
JSON object containing meta data. As long as data sources are neither
added nor removed, the meta data doesn't change and only the binary data
is updated. When a data source is added or removed, the existing buffer
containing binary and meta data is invalidated, recomputed and gets
written out.

# Data Layout

The data layout is not defined by this library but by the existing
daemon: https://github.com/xapi-project/rrd-transport/.

    value bits format notes

    header string (string length) * 8 string
    "DATASOURCES"

    data checksum 32 int32
    binary-encoded crc32 of the concatenation of the encoded timestamp
    and datasource values

    metadata checksum 32 int32
    binary-encoded crc32 of the metadata string (see below)

    number of datasources 32 int32
    only needed if the metadata has changed - otherwise RRDD can use a
    cached value

    timestamp 64 int64 Unix epoch

    datasource values n * 64 int64
    n is the number of datasources exported by the plugin

    metadata length 32 int32

    metadata (string length) * 8 string

All integers are in big endian.

    00000000: 4441 5441 534f 5552 4345 53b2 49c2 820e  DATASOURCES.I...
    00000010: eade 5100 0000 0100 0000 0057 9210 4700  ..Q........W..G.
    00000020: 0000 0057 9210 4700 0000 b27b 2264 6174  ...W..G....{"dat
    00000030: 6173 6f75 7263 6573 223a 7b22 6375 7272  asources":{"curr
    00000040: 656e 745f 7469 6d65 223a 7b22 6465 7363  ent_time":{"desc
    00000050: 7269 7074 696f 6e22 3a22 5468 6520 6375  ription":"The cu
    00000060: 7272 656e 7420 7469 6d65 222c 226f 776e  rrent time","own
    00000070: 6572 223a 2268 6f73 7422 2c22 7661 6c75  er":"host","valu
    00000080: 655f 7479 7065 223a 2269 6e74 3634 222c  e_type":"int64",
    00000090: 2274 7970 6522 3a22 6761 7567 6522 2c22  "type":"gauge","
    000000a0: 6465 6661 756c 7422 3a22 7472 7565 222c  default":"true",
    000000b0: 2275 6e69 7473 223a 2273 6563 6f6e 6473  "units":"seconds
    000000c0: 222c 226d 696e 223a 222d 696e 6622 2c22  ","min":"-inf","
    000000d0: 6d61 7822 3a22 696e 6622 7d7d 7d00 0000  max":"inf"}}}...
    000000e0: 0000 0000 0000 0000 0000 0000 0000 0000  ................

    {
      "datasources": {
        "current_time": {
          "description": "The current time",
          "owner": "host",
          "value_type": "int64",
          "type": "gauge",
          "default": "true",
          "units": "seconds",
          "min": "-inf",
          "max": "inf"
        }
      }
    }

## License

MIT License.

