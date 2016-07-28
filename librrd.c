/*
 * vim: set ts=4 sw=4 noet:
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

#include "librrd.h"
#include "parson/parson.h"

#define MAGIC "DATASOURCES"
#define MAGIC_SIZE (sizeof (MAGIC)-1)


struct rrd_header {
    char            rrd_magic[MAGIC_SIZE];
    int32_t         rrd_checksum_value;
    int32_t         rrd_checksum_meta;
    int32_t         rrd_header_datasources;
    int64_t         rrd_timestamp;
} __attribute__ ((packed)) __;
typedef struct rrd_header RRD_HEADER;

uint64_t
htonll(const uint64_t in)
{
    unsigned char   out[8] =
        { in >> 56, in >> 48, in >> 40, in >> 32, in >> 24, in >> 16,
in >> 8, in };
    return *(uint64_t *) out;
}

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

    size_t          i;
    for (i = 0; i < RRD_MAX_SOURCES; i++) {
        JSON_Value     *src;
        if (plugin->sources[i] == NULL)
            continue;
        src = json_for_source(plugin->sources[i]);
        json_object_set_value(root, plugin->sources[i]->name, src);
    }

#if 0
    char           *pretty;
    pretty = json_serialize_to_string_pretty(json);
    puts(pretty);
    json_free_serialized_string(pretty);

#endif

    return json;
}

static void
initialise(RRD_PLUGIN * plugin)
{

    RRD_HEADER     *header;
    int32_t         size_meta;
    int32_t         size_total;
    int64_t         *p64;
    int32_t         *p32;

    assert(plugin);
    assert(plugin->meta == NULL);
    assert(plugin->buf == NULL);

    plugin->meta = json_for_plugin(plugin);
    size_meta = json_serialization_size_pretty(plugin->meta);

    size_total = 0;
    size_total += sizeof(RRD_HEADER);
    size_total += plugin->n * sizeof(int64_t);
    size_total += sizeof(int32_t);
    size_total += size_meta;

    plugin->buf_size = size_total;
    plugin->buf = malloc(size_total);
    if (!plugin->buf) {
        perror("buffer_init");
        exit(1);
    }

    /*
     * all values are in network byte order 
     */
    header = (RRD_HEADER *) plugin->buf;
    memcpy(&header->rrd_magic, MAGIC, MAGIC_SIZE);
    header->rrd_checksum_value = htonl(0x0123);
    header->rrd_checksum_meta = htonl(0x4567);
    header->rrd_header_datasources = htonl(plugin->n);
    header->rrd_timestamp = 0;

    p64 = (int64_t*) (plugin->buf + sizeof(RRD_HEADER));
    for (size_t i = 0; i < plugin->n; i++) {
        *p64++ = htonll(0x8888);
    }
    p32 = (int32_t*) p64;
    *p32++ = htonl(size_meta);

    printf("slots         = %d\n", plugin->n);
    printf("fixed header  = %ld\n", sizeof(RRD_HEADER));
    printf("binary header = %ld\n", ((char*)p32)-plugin->buf);
    printf("size_total    = %d\n", size_total);
    printf("size_meta     = %d\n", size_meta);

    json_serialize_to_buffer_pretty(plugin->meta, (char*) p32, size_meta);

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
    plugin->json = NULL;
    for (int i = 0; i < RRD_MAX_SOURCES; i++) {
        plugin->sources[i] = (RRD_SOURCE *) NULL;
    }
    plugin->n = 0;
    plugin->buf_size = 0;
    plugin->buf = NULL;
    plugin->meta = NULL;

    plugin->file = open(path, O_RDWR | O_CREAT | O_APPEND,
            S_IRUSR|S_IWUSR);
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

    json = json_for_plugin(plugin);
    json_value_free(json);

    if (plugin->buf == NULL) {
        initialise(plugin);
    }

    for (size_t i = 0; i < RRD_MAX_SOURCES; i++) {
        if (plugin->sources[i] == NULL)
            continue;
        n++;
        plugin->sources[i]->sample();
    }
    /* must have exactly n data sources */
    assert (n == plugin->n);

    /* write out buffer */
    int             written = 0;
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
