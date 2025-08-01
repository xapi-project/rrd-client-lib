/*
 * Copyright (c) 2016 Citrix
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "librrd.h"
#include "parson/parson.h"

#define MAGIC "DATASOURCES"
#define MAGIC_SIZE (sizeof (MAGIC)-1)
#define RRD_MAX_JSON (2048 * RRD_MAX_SOURCES)

#ifndef __APPLE__
#include <endian.h>
#define htonll(x) htobe64(x)
#endif

/*
 * The type RRD_PLUGIN below is private to the implementation and entirely
 * managed by it.
 */

struct rrd_plugin {
    char           *name;       /* name of the plugin */
    char           *path;       /* path to file */
    RRD_SOURCE     *sources[RRD_MAX_SOURCES];
    JSON_Value     *meta;       /* meta data for the plugin */
    char           *buf;        /* buffer where we keep protocol data */
    rrd_domain_t    domain;     /* domain of this plugin */
    uint32_t        n;          /* number of used slots */
    size_t          buf_size;   /* size of the buffer */
    int             file;       /* where we report data */
};


/*
 * The struct represents the first 31 bytes in the protocol header. Fields
 * are not 4-byte or 8-byte aligned which is why we need the packed
 * attribute.
 */
struct rrd_header {
    uint8_t         rrd_magic[MAGIC_SIZE];
    uint32_t        rrd_checksum_value;
    uint32_t        rrd_checksum_meta;
    uint32_t        rrd_header_datasources;
    uint64_t        rrd_timestamp;
    uint32_t        rrd_data[];/* more data follows */
}               __attribute__((packed));
typedef struct rrd_header RRD_HEADER;

/*
 * write data to fd
 */
static int
write_exact(int fd, const void *data, size_t size)
{
    size_t          offset = 0;
    ssize_t         len;
    while (offset < size) {
        len = write(fd, (const char *)data + offset, size - offset);
        if ((len == -1) && (errno == EINTR))
            continue;
        if (len <= 0)
            return -1;
        offset += len;
    }
    return 0;
}

/*
 * invalidate the current buffer. It will be re-created by sample().
 */
static void
invalidate(RRD_PLUGIN * plugin)
{
    assert(plugin);

    free(plugin->buf);
    plugin->buf = NULL;
    plugin->buf_size = 0;
    json_value_free(plugin->meta);
    plugin->meta = NULL;
}

/*
 * Generate JSON for a data source and return it as an JSON object (from
 * where it can be rendered to a string.
 */
static JSON_Value *
json_for_source(RRD_SOURCE * source)
{
    assert(source);

    JSON_Value     *json = json_value_init_object();
    JSON_Object    *src = json_value_get_object(json);

    json_object_set_string(src, "description", source->description);
    json_object_set_string(src, "units", source->rrd_units);
    json_object_set_string(src, "min", source->min);
    json_object_set_string(src, "max", source->max);
    json_object_set_string(src, "default",
                           source->rrd_default ? "true" : "false");

    char           owner[128] = {0};

    switch (source->owner) {
    case RRD_HOST:
        snprintf(owner, sizeof owner, "host");
        break;
    case RRD_VM:
        snprintf(owner, sizeof owner, "vm %s", source->owner_uuid);
        break;
    case RRD_SR:
        snprintf(owner, sizeof owner, "sr %s", source->owner_uuid);
        break;
    default:
        abort();
    }
    json_object_set_string(src, "owner", owner);

    char           *value_type = NULL;
    switch (source->type) {
    case RRD_INT64:
        value_type = "int64";
        break;
    case RRD_FLOAT64:
        value_type = "float";
        break;
    default:
        abort();
    }
    json_object_set_string(src, "value_type", value_type);

#define RRD_TRANSPORT_1_1_0
#ifdef RRD_TRANSPORT_1_1_0
#define GAUGE "gauge"
#define ABSOLUTE "absolute"
#define DERIVE "derive"
#else
#define GAUGE "absolute"
#define ABSOLUTE "rate"
#define DERIVE "absolute_to_rate"
#endif

    char           *scale = NULL;
    switch (source->scale) {
    case RRD_GAUGE:
        scale = GAUGE;
        break;
    case RRD_ABSOLUTE:
        scale = ABSOLUTE;
        break;
    case RRD_DERIVE:
        scale = DERIVE;
        break;
    default:
        abort();
    }
    json_object_set_string(src, "type", scale);

    return json;
}
/*
 * Generate JSON for a plugin. This is just a JSON object containing a
 * sub-object for every data source.
 */
static JSON_Value *
json_for_plugin(RRD_PLUGIN * plugin)
{
    assert(plugin);

    JSON_Value     *root_json = json_value_init_object();
    JSON_Object    *root = json_value_get_object(root_json);

    JSON_Value     *ds_json = json_value_init_object();
    JSON_Object    *ds = json_value_get_object(ds_json);

    json_object_set_value(root, "datasources", ds_json);
    for (size_t i = 0; i < RRD_MAX_SOURCES; i++) {
        JSON_Value     *src;
        if (plugin->sources[i] == NULL)
            continue;
        src = json_for_source(plugin->sources[i]);
        json_object_set_value(ds, plugin->sources[i]->name, src);
    }
    return root_json;
}

double get_timestamp()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (double) tp.tv_sec + (double) tp.tv_usec / 1e6;
}

/**
 * Returns the IEEE 754 bit pattern of a double as a uint64_t.
 * The timestamp used in RRD was changed from int to double to increase
 * precision. In librrd, htonll() is used to convert the timestamp from
 * little-endian to big-endian. However, htonll() requires an uint64_t
 * parameter, so we first need to convert the double to an uint64_t in
 * order to obtain its bit representation as an uint64_t.
 */
uint64_t bits_of_double(double d)
{
    int64_t i;
    memcpy(&i, &d, sizeof(d));
    return i;
}

/*
 * initialise the buffer that we update and write out to a file. Once
 * initialised, it is kept up to date by sample().
 */
static int
initialise(RRD_PLUGIN * plugin)
{

    RRD_HEADER     *header;
    uint32_t        size_meta;
    uint32_t        size_total;
    int64_t        *p64;
    int32_t        *p32;

    assert(plugin);
    assert(plugin->meta == NULL);
    assert(plugin->buf == NULL);
    assert(plugin->n <= RRD_MAX_SOURCES);

    plugin->meta = json_for_plugin(plugin);
    size_meta = json_serialization_size_pretty(plugin->meta);
    assert(size_meta < RRD_MAX_JSON);   /* just a safeguard */

    size_total = 0;
    size_total += sizeof(RRD_HEADER);
    size_total += RRD_MAX_SOURCES * sizeof(int64_t);
    size_total += sizeof(uint32_t);
    size_total += RRD_MAX_JSON;

    plugin->buf_size = size_total;
    plugin->buf = calloc(1, size_total);
    if (!plugin->buf) {
        /*
         * fatal
         */
        plugin->buf_size = 0;
        return -1;
    }
    /*
     * all values need to be in network byte order
     */
    header = (RRD_HEADER *) plugin->buf;
    memcpy(&header->rrd_magic, MAGIC, MAGIC_SIZE);
    header->rrd_checksum_value = htonl(0x01234567);
    header->rrd_header_datasources = htonl(plugin->n);
    header->rrd_timestamp = htonll(bits_of_double(get_timestamp()));
    if (header->rrd_timestamp == -1) {
        free(plugin->buf);
        plugin->buf_size = 0;
        plugin->buf = NULL;
        return -1;
    }
    p64 = (int64_t *) (plugin->buf + sizeof(RRD_HEADER));
    for (size_t i = 0; i < plugin->n; i++) {
        *p64++ = htonll(0x0011223344556677);
    }
    p32 = (int32_t *) p64;
    *p32++ = htonl(size_meta);
    json_serialize_to_buffer_pretty(plugin->meta, (char *)p32, size_meta);

    uint32_t        crc;
    crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (unsigned char *)p32, size_meta);
    header->rrd_checksum_meta = htonl(crc);
    return 0;
}

/*
 * rrd_open creates the data structure that represents a plugin with
 * initially no data source. Data sources will be added later by rrd_add_src.
 */
RRD_PLUGIN     *
rrd_open(char *name, rrd_domain_t domain, char *path)
{
    assert(name);
    assert(path);

    RRD_PLUGIN     *plugin = malloc(sizeof(RRD_PLUGIN));
    if (!plugin) {
        return NULL;
    }
    plugin->name = name;
    plugin->path = path;
    plugin->domain = domain;
    /*
     * mark all slots for data sources as free
     */
    for (int i = 0; i < RRD_MAX_SOURCES; i++) {
        plugin->sources[i] = (RRD_SOURCE *) NULL;
    }
    plugin->n = 0;
    plugin->buf_size = 0;
    plugin->buf = NULL;
    plugin->meta = NULL;

    if (initialise(plugin) != 0) {
        invalidate(plugin);
        free(plugin);
        return NULL;
    }
    plugin->file = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (plugin->file == -1) {
        invalidate(plugin);
        free(plugin);
        return NULL;
    }
    if (write_exact(plugin->file, plugin->buf, plugin->buf_size) != 0) {
        invalidate(plugin);
        free(plugin);
        return NULL;
    }
    return plugin;
}

/*
 * unregister a plugin. Free all resources that we have allocted. Note that
 * calling free(NULL) is fine in case some resource was already de-allocated.
 */
int
rrd_close(RRD_PLUGIN * plugin)
{
    assert(plugin);
    int             rc;


    rc = close(plugin->file);
    if (rc == 0)
        rc = unlink(plugin->path);
    invalidate(plugin);
    free(plugin);
    return (rc == 0 ? RRD_OK : RRD_FILE_ERROR);
}
/*
 * Add a new data source to a plugin. It is inserted into the first free slot
 * available.
 */
int
rrd_add_src(RRD_PLUGIN * plugin, RRD_SOURCE * source)
{
    assert(plugin);
    assert(source);

    /*
     * find free slot
     */
    size_t          i;
    for (i = 0; i < RRD_MAX_SOURCES; i++) {
        if (plugin->sources[i] == NULL)
            break;
    }
    if (i >= RRD_MAX_SOURCES) {
        return RRD_TOO_MANY_SOURCES;
    }
    plugin->sources[i] = source;
    plugin->n++;
    invalidate(plugin);

    return RRD_OK;
}

/*
 * Remove a previously registered data source from a plugin,
 */
int
rrd_del_src(RRD_PLUGIN * plugin, RRD_SOURCE * source)
{
    assert(source);
    size_t          i;
    /*
     * find slot with source
     */
    for (i = 0; i < RRD_MAX_SOURCES; i++) {
        if (plugin->sources[i] == source)
            break;
    }
    if (i >= RRD_MAX_SOURCES) {
        return RRD_NO_SUCH_SOURCE;
    }
    plugin->sources[i] = NULL;
    plugin->n--;
    invalidate(plugin);

    return RRD_OK;
}

/*
 * Sample obtains a values form all data sources by calling their sample
 * functions. It updates the buffer with all data and writes it out. If there
 * is no meta data it means it was invalidated previously because a data
 * source was added or removed. In that case in creates a new buffer first.
 */
int
rrd_sample(RRD_PLUGIN * plugin, time_t(*t) (time_t *))
{
    assert(plugin);
    int             n = 0;
    int64_t        *p;
    RRD_HEADER     *header;

    if (plugin->buf == NULL) {
        int             rc;
        rc = initialise(plugin);
        if (rc != 0) {
            return RRD_ERROR;
        }
    }
    assert(plugin->buf);
    header = (RRD_HEADER *) plugin->buf;

    /*
     * sample n sources and write values to buffer
     */
    p = (int64_t *) (plugin->buf + sizeof(RRD_HEADER));
    for (size_t i = 0; i < RRD_MAX_SOURCES; i++) {
        void *userdata;
        if (plugin->sources[i] == NULL)
            continue;
        userdata = plugin->sources[i]->userdata;

        rrd_value_t     v = plugin->sources[i]->sample(userdata);
        *p++ = htonll((uint64_t) v.int64);
        n++;
    }
    /*
     * must have sampled exactly n data sources
     */
    assert(n == plugin->n);

    /*
     * update timestamp, calculate crc
     */

    header->rrd_timestamp = htonll(bits_of_double(get_timestamp()));
    uint32_t        crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc,
                (unsigned char *)&header->rrd_timestamp,
                (n + 1) * sizeof(int64_t));
    header->rrd_checksum_value = htonl(crc);

    /*
     * reset file pointer, write out buffer
     */
    if (lseek(plugin->file, 0, SEEK_SET) < 0) {
        return RRD_FILE_ERROR;
    }
    if (write_exact(plugin->file, plugin->buf, plugin->buf_size) != 0) {
        return RRD_FILE_ERROR;
    }
    return RRD_OK;
}
