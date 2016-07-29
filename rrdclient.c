
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include "librrd.h"

static RRD_SOURCE src;
static rrd_value v;

static          rrd_value
sample(void)
{
    printf("sample called: %ld\n", v.int64);
    return v;
}


int
main(int argc, char **argv)
{
    RRD_PLUGIN     *plugin;
    char            line[256];

    if (argc != 2) {
        fprintf(stderr, "usage: %s file.rrd\n", basename(argv[0]));
        exit(1);
    }

    plugin = rrd_open("client", RRD_LOCAL_DOMAIN, argv[1]);
    if (!plugin) {
        fprintf(stderr, "can't open %s\n", argv[1]);
        exit(1);
    }
    rrd_add_src(plugin, &src);

    src.name = "stdin";
    src.description = "integers read from stdin";
    src.owner = RRD_HOST;
    src.owner_uuid = "931388d6-559e-11e6-ab0a-73658ca1c515";
    src.rrd_units = "numbers";
    src.scale = RRD_GAUGE;
    src.min = "-inf";
    src.max = "inf";
    src.rrd_default = 0;
    src.sample = sample;

    while (fgets(line, sizeof(line), stdin) != NULL) {
        v.int64 = (int64_t) atol(line);
        rrd_sample(plugin);
    }

    rrd_del_src(plugin, &src);
    rrd_close(plugin);
    return 0;
}
