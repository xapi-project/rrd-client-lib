/*
 * Copyright (c) 2016 Citrix
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <stdint.h>
#include "parson/parson.h"

#define RRD_MAX_SOURCES         128

#define RRD_OK                  0
#define RRD_TOO_MANY_SOURCES    1
#define RRD_NO_SUCH_SOURCE      2
#define RRD_FILE_ERROR          3
#define RRD_ERROR               4

typedef enum { RRD_LOCAL_DOMAIN = 0, RRD_INTER_DOMAIN } rrd_domain;
typedef enum { RRD_GAUGE = 0, RRD_ABSOLUTE, RRD_DERIVE } rrd_scale;
typedef enum { RRD_HOST = 0, RRD_VM, RRD_SR } rrd_owner;
typedef enum { RRD_FLOAT64 = 0, RRD_INT64 } rrd_type;

/*
 * An RRD_SOURCE can report float or integer values - represented as a
 * rrd_value. The type component tells which one it is.
 */
typedef union {
    int64_t         int64;
    float           float64;
} rrd_value;

/*
 * An RRD_SOURCE describes the value being reported and contains a
 * sample() function to obtain such values. Strings are expected
 * to be in UTF8 encoding.
 */
typedef struct rrd_source {
    char           *name;       /* name of the data source */
    char           *description;        /* for user interface */
    rrd_owner       owner;
    int             rrd_default;        /* true: rrd daemon will archive */
    char           *owner_uuid; /* UUID of the owner or NULL */
    char           *rrd_units;  /* for user interface */
    rrd_scale       scale;      /* presentation of value */
    rrd_type        type;       /* type of value */
    char           *min;        /* min <= sample() <= max */
    char           *max;        /* min <= sample() <= max */
                    rrd_value(*sample) (void);  /* reads value that gets *
                                                 * reported */
} RRD_SOURCE;

/*
 * The type RRD_PLUGIN below is private to the implementation and
 * entirely managed by it.
 */

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


/*
 * Memory management policy: the library does not free the memory of any
 * objects that are passed into it (like strings or RRD_SOURCE objects).
 * It does free any memory that it allocates internally when calling
 * rrd_close().
 */

/*
 * rrd_open - regsiter a new plugin
 * name: name of the plugin in UTF8 encoding.
 * domain: INTER_DOMAIN if it reports data from multiple domains
 * path: file path where the plugin writes it samples
 * returns:
 * NULL on error
 */
RRD_PLUGIN     *rrd_open(char *name, rrd_domain domain, char *path);

/*
 * rrd_close - close a plugin. Data sources do not need to be removed
 * from the plugin before calling rrd_close.
 */
int             rrd_close(RRD_PLUGIN * plugin);

/*
 * rrd_add_src - add a new data source returns: error code At most
 * RRD_MAX_SOURCES can be active. It is an unchecked error to register the
 * same source multiple times. The name of the source must be unique for
 * all sources added to a plugin.
 */
int             rrd_add_src(RRD_PLUGIN * plugin, RRD_SOURCE * source);

/*
 * rrd_del_src - remove a data source. Returns an error code.
 */
int             rrd_del_src(RRD_PLUGIN * plugin, RRD_SOURCE * source);

/*
 * calling rrd_sample(plugin) triggers that all data sources are sampled
 * and the results are reported to the RRD daemon. This function needs to
 * be called every 5 seconds by the client,
 */
int             rrd_sample(RRD_PLUGIN * plugin);
