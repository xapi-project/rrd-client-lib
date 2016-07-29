/*

Copyright (c) 2016 Citrix

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <arpa/inet.h>

#include "librrd.h"
#include "parson/parson.h"

#define MAGIC "DATASOURCES"
#define MAGIC_SIZE (sizeof (MAGIC)-1)

#ifndef DARWIN
#include <endian.h>
#define htonll(x) htobe64(x)
#endif

struct rrd_header {
    char            rrd_magic[MAGIC_SIZE];
    uint32_t        rrd_checksum_value;
    uint32_t        rrd_checksum_meta;
    uint32_t        rrd_header_datasources;
    uint64_t        rrd_timestamp;
} __attribute__ ((packed));
typedef struct rrd_header RRD_HEADER;

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
    json_object_set_boolean(src, "default", source->rrd_default);

    char           *owner = NULL;
    switch (source->owner) {
    case RRD_HOST:
        owner = "host";
        break;
    case RRD_VM:
        owner = "vm";
        break;
    case RRD_SR:
        owner = "rrd";
        break;
    default:
        assert(0);
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
        assert(0);
    }
    json_object_set_string(src, "value_type", value_type);


    char           *scale = NULL;
    switch (source->scale) {
    case RRD_GAUGE:
        scale = "gauge";
        break;
    case RRD_ABSOLUTE:
        scale = "absolute";
        break;
    case RRD_DERIVE:
        scale = "derive";
        break;
    default:
        assert(0);
    }
    json_object_set_string(src, "type", scale);

    return json;
}

static JSON_Value *
json_for_plugin(RRD_PLUGIN * plugin)
{
    assert(plugin);

    JSON_Value     *json = json_value_init_object();
    JSON_Object    *root = json_value_get_object(json);
    JSON_Value     *src;

    for (size_t i = 0; i < RRD_MAX_SOURCES; i++) {
        if (plugin->sources[i] == NULL)
            continue;
        src = json_for_source(plugin->sources[i]);
        json_object_set_value(root, plugin->sources[i]->name, src);
    }
    return json;
}

static void
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

    plugin->meta = json_for_plugin(plugin);
    size_meta = json_serialization_size_pretty(plugin->meta);

    size_total = 0;
    size_total += sizeof(RRD_HEADER);
    size_total += plugin->n * sizeof(int64_t);
    size_total += sizeof(uint32_t);
    size_total += size_meta;

    plugin->buf_size = size_total;
    plugin->buf = malloc(size_total);
    if (!plugin->buf) {
        perror("buffer_init");
        exit(1);
    }

    /*
     * all values need to be in network byte order
     */
    header = (RRD_HEADER *) plugin->buf;
    memcpy(&header->rrd_magic, MAGIC, MAGIC_SIZE);
    header->rrd_checksum_value = htonl(0x01234567);
    header->rrd_header_datasources = htonl(plugin->n);
    header->rrd_timestamp = htonll((uint64_t) time(NULL));

    p64 = (int64_t *) (plugin->buf + sizeof(RRD_HEADER));
    for (size_t i = 0; i < plugin->n; i++) {
        *p64++ = htonll(0x001122334455667788);
    }
    p32 = (int32_t *) p64;
    *p32++ = htonl(size_meta);

    printf("slots         = %d\n", plugin->n);
    printf("fixed header  = %ld\n", sizeof(RRD_HEADER));
    printf("binary header = %ld\n", ((char *) p32) - plugin->buf);
    printf("size_total    = %d\n", size_total);
    printf("size_meta     = %d\n", size_meta);

    json_serialize_to_buffer_pretty(plugin->meta, (char *) p32, size_meta);

    uint32_t        crc;
    crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (unsigned char *) p32, size_meta);
    header->rrd_checksum_meta = htonl(crc);
}

RRD_PLUGIN     *
rrd_open(char *name, rrd_domain domain, char *path)
{
    assert(name);
    assert(path);

    RRD_PLUGIN     *plugin = (RRD_PLUGIN *) malloc(sizeof(RRD_PLUGIN));
    if (!plugin) {
        perror("rrd_open");
        exit(RRD_ERROR);
    }

    plugin->name = name;
    plugin->domain = domain;
    for (int i = 0; i < RRD_MAX_SOURCES; i++) {
        plugin->sources[i] = (RRD_SOURCE *) NULL;
    }
    plugin->n = 0;
    plugin->buf_size = 0;
    plugin->buf = NULL;
    plugin->meta = NULL;

    plugin->file = open(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    if (!plugin->file) {
        perror("rrd_open");
        exit(RRD_FILE_ERROR);
    }

    return plugin;
}

int
rrd_close(RRD_PLUGIN * plugin)
{
    assert(plugin);

    if (close(plugin->file) != 0) {
        perror("rrd_close");
        exit(RRD_FILE_ERROR);
    }
    json_value_free(plugin->meta);
    free(plugin->buf);
    free(plugin);
    return RRD_OK;
}

int
rrd_add_src(RRD_PLUGIN * plugin, RRD_SOURCE * source)
{
    assert(plugin);
    assert(source);

    /*
     * find free slot
     */
    size_t          i = 0;
    while (plugin->sources[i] != NULL && i < RRD_MAX_SOURCES)
        i++;
    if (i >= RRD_MAX_SOURCES) {
        return RRD_TOO_MANY_SOURCES;
    }
    plugin->sources[i] = source;
    plugin->n++;
    invalidate(plugin);

    return RRD_OK;
}

int
rrd_del_src(RRD_PLUGIN * plugin, RRD_SOURCE * source)
{
    assert(source);
    size_t          i = 0;
    /*
     * find source in plugin
     */
    while (i < RRD_MAX_SOURCES && plugin->sources[i] != source)
        i++;
    if (i >= RRD_MAX_SOURCES) {
        return RRD_NO_SOUCH_SOURCE;
    }
    plugin->sources[i] = NULL;
    plugin->n--;
    invalidate(plugin);

    return RRD_OK;
}


int
rrd_sample(RRD_PLUGIN * plugin)
{
    assert(plugin);
    JSON_Value     *json;
    int n = 0;
    int64_t *p;
    RRD_HEADER *header;

    json = json_for_plugin(plugin);
    json_value_free(json);

    if (plugin->buf == NULL) {
        initialise(plugin);
    }
    assert(plugin->buf);
    header = (RRD_HEADER*) plugin->buf;

    /* sample n sources and write values to buffer */
    p = (int64_t*)(plugin->buf + sizeof(RRD_HEADER));
    for (size_t i = 0; i < RRD_MAX_SOURCES; i++) {
        if (plugin->sources[i] == NULL)
            continue;
        rrd_value v = plugin->sources[i]->sample();
        *p++ = htonll((uint64_t)v.int64);
        n++;
    }
    /* must have sampled exactly n data sources */
    assert (n == plugin->n);

    /* update timestamp, calculate crc */
    header->rrd_timestamp = htonll((uint64_t)time(NULL));
    uint32_t crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc,
            (unsigned char*) &header->rrd_timestamp,
            (n+1)*sizeof(int64_t));
    header->rrd_checksum_value = htonl(crc);

    /* write out buffer */
    ssize_t             written = 0;
    while (written >= 0 && written < plugin->buf_size)
        written += write(plugin->file,
                         plugin->buf + written,
                         plugin->buf_size - written);
    if (written < 0) {
        perror("rrd_sample");
        return RRD_FILE_ERROR;
    }

    return RRD_OK;
}
