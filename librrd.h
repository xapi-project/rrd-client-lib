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
#include <time.h>
#include "parson/parson.h"

#define RRD_MAX_SOURCES         16

#define RRD_OK                  0
#define RRD_TOO_MANY_SOURCES    1
#define RRD_NO_SUCH_SOURCE      2
#define RRD_FILE_ERROR          3
#define RRD_ERROR               4

/* rrd_domain_t */
typedef int32_t rrd_domain_t;
#define RRD_LOCAL_DOMAIN        0
#define RRD_INTER_DOMAIN        1

/* rrd_scale_t */
typedef int32_t rrd_scale_t;
#define RRD_GAUGE               0
#define RRD_ABSOLUTE            1
#define RRD_DERIVE              2

/* rrd_owner_t */
typedef int32_t rrd_owner_t;
#define RRD_HOST                0
#define RRD_VM                  1
#define RRD_SR                  2

/* rrd_type_t */
typedef int32_t rrd_type_t;
#define RRD_FLOAT64             0
#define RRD_INT64               1


/*
 * An RRD_SOURCE can report float or integer values - represented as a
 * rrd_value. The type component tells which one it is.
 */
typedef union {
    int64_t         int64;
    double          float64;
} rrd_value_t;

/*
 * An RRD_SOURCE describes the value being reported and contains a
 * sample() function to obtain such values. Strings are expected
 * to be in UTF8 encoding. Part of an RRD_SOURCE is a pointer userdata
 * that is passed to sample(). This allows to share a single sample
 * function across several RRD_SOURCE values.
 */
typedef struct rrd_source {
    char           *name;       /* name of the data source */
    char           *description;        /* for user interface */
    char           *owner_uuid; /* UUID of the owner or NULL */
    char           *rrd_units;  /* for user interface */
    char           *min;        /* min <= sample() <= max */
    char           *max;        /* min <= sample() <= max */
                    rrd_value_t(*sample) (void *userdata);      /* reads
                                                                 * value */
    void           *userdata;   /* passed to sample() */
    rrd_owner_t     owner;
    int32_t         rrd_default;        /* true: rrd daemon will archive */
    rrd_scale_t     scale;      /* presentation of value */
    rrd_type_t      type;       /* type of value */
} RRD_SOURCE;
typedef struct rrd_plugin RRD_PLUGIN;

/*
 * Memory management policy: the library does not free the memory of any
 * objects that are passed into it (like strings or RRD_SOURCE objects).
 * It does free any memory that it allocates internally when calling
 * rrd_close().
 */

/*
 * rrd_open - register a new plugin
 * name: name of the plugin in UTF8 encoding.
 * domain: INTER_DOMAIN if it reports data from multiple domains
 * path: file path where the plugin writes it samples
 * returns:
 * NULL on error
 */
RRD_PLUGIN     *rrd_open(char *name, rrd_domain_t domain, char *path);

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
 *
 *
 */
int             rrd_add_src(RRD_PLUGIN * plugin, RRD_SOURCE * source);

/*
 * rrd_del_src - remove a data source. Returns an error code.
 */
int             rrd_del_src(RRD_PLUGIN * plugin, RRD_SOURCE * source);

/*
 * calling rrd_sample(plugin) triggers that all data sources are sampled
 * and the results are reported to the RRD daemon. This function needs
 * to be called every 5 seconds by the client. The second parameter is
 * typically NULL. If it isn't, it will be used instead of time(3) to
 * obtain the time stamp that is written to the RRD file. This can be
 * used to create RRD files that don't depend on the current time.
 */
int             rrd_sample(RRD_PLUGIN * plugin, time_t (*t)(time_t*));
