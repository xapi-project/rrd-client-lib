/*
 * vim: set ts=8 sw=8 noet: 
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

static          size_t
rrd_buffer_size_source(RRD_SOURCE * source)
{
    return 0;
}

static          size_t
rrd_buffer_size(RRD_PLUGIN * plugin)
{
    assert(plugin);

    size_t          size = 0;

    return 0;
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

    for (int i = 0; i < RRD_MAX_SOURCES; i++) {
        free(plugin->sources[i]);
    }
    if (close(plugin->file) != 0) {
        perror("rrd_close");
        exit(RRD_FILE_ERROR);
    }
    return RRD_OK;
}

int
rrd_add_src(RRD_PLUGIN * plugin, RRD_SOURCE * source)
{
    assert(plugin);
    assert(source);

    return RRD_OK;
}

int
rrd_del_src(RRD_PLUGIN * plugin, RRD_SOURCE * source)
{
    assert(source);
    return RRD_OK;
}

int
rrd_sample(RRD_PLUGIN * plugin)
{
    assert(plugin);

    return RRD_OK;
}
