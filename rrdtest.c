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


#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <assert.h>

#include "librrd.h"


static int64_t  numbers[] =
    { 2, 16, 28, 29, 29, 34, 40, 48, 49, 52, 54, 55, 55, 57, 66, 67, 83,
    85, 90, 97
};

/* Return the next value from the numbers array. sample keeps an
 * internal index into the array that wraps around when it reaches the
 * end.
 */

static          rrd_value
sample(void)
{
    rrd_value       v;
    static size_t   i = 0;

    v.int64 = numbers[i++ % (sizeof(numbers) / sizeof(numbers[0]))];

    printf("sample called: %ld\n", v.int64);
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
    src[0].rrd_default = 1;
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
    src[1].rrd_default = 1;
    src[1].sample = sample;
    rrd_add_src(plugin, &src[1]);
    rrd_sample(plugin);

    rrd_del_src(plugin, &src[0]);
    rrd_sample(plugin);

    rrd_del_src(plugin, &src[1]);
    rrd_close(plugin);
    return 0;
}
