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


#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include "librrd.h"
#include "assert.h"

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

    plugin = rrd_open(argv[0], RRD_LOCAL_DOMAIN, argv[1]);
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

    int rc;
    while (fgets(line, sizeof(line), stdin) != NULL) {
        v.int64 = (int64_t) atol(line);
        rc = rrd_sample(plugin);
        assert(rc == RRD_OK);
    }

    rrd_del_src(plugin, &src);
    rrd_close(plugin);
    return 0;
}
