/*
 *
 * vim: set ts=4 sw=4 et: 
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <assert.h>

#include "librrd.h"

static          rrd_value
sample(void)
{
    rrd_value       v;
    v.int64 = 0;

    printf("sample called\n");
    return v;
}

static RRD_SOURCE src[2];

int
main(int argc, char **argv)
{
    RRD_PLUGIN     *plugin;

    if (argc != 1) {
        fprintf(stderr, "usage: %s\n", basename(argv[0]));
        exit(1);
    }

    plugin = rrd_open("rrdtest", RRD_LOCAL_DOMAIN, "rrdtest.rrd");
    assert(plugin);

    src[0].name = "first";
    src[0].description = "description";
    src[0].owner = RRD_HOST;
    src[0].owner_uuid = "4cc1f2e0-5405-11e6-8c2f-572fc76ac144";
    src[0].rrd_units = "points";
    src[0].scale = RRD_GAUGE;
    src[0].min = "-inf";
    src[0].max = "inf";
    src[0].sample = sample;

    rrd_add_src(plugin, &src[0]);
    rrd_sample(plugin);

    src[1].name = "second";
    src[1].description = "description";
    src[1].owner = RRD_HOST;
    src[1].owner_uuid = "e8969702-5414-11e6-8cf5-47824be728c3";
    src[1].rrd_units = "points";
    src[1].scale = RRD_GAUGE;
    src[1].min = "-inf";
    src[1].max = "inf";
    src[1].sample = sample;
    rrd_add_src(plugin, &src[1]);
    rrd_sample(plugin);

    rrd_del_src(plugin, &src[0]);
    rrd_sample(plugin);

    rrd_close(plugin);
    return 0;
}
