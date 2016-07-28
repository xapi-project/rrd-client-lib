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

    char           *pretty;
    pretty = json_serialize_to_string_pretty(json);
    puts(pretty);
    json_free_serialized_string(pretty);

    return json;
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
    plugin->dirty = 1;
    plugin->n = 0;

    plugin->file = open(path, O_WRONLY | O_CREAT);
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

    assert(plugin->n == 0);
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
    plugin->dirty = 1;
    plugin->n++;

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
    plugin->dirty = 1;
    plugin->n--;
    return RRD_OK;
}


int
rrd_sample(RRD_PLUGIN * plugin)
{
    assert(plugin);
    JSON_Value     *json;

    json = json_for_plugin(plugin);
    json_value_free(json);


    for (size_t i = 0; i < RRD_MAX_SOURCES; i++) {
        if (plugin->sources[i] == NULL)
            continue;
        plugin->sources[i]->sample();
    }
    return RRD_OK;
}
