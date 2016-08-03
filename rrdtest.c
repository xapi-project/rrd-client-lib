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
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>

#include "librrd.h"

static int64_t  numbers[] =
    { 2, 16, 28, 29, 29, 34, 40, 48, 49, 52, 54, 55, 55, 57, 66, 67, 83,
    85, 90, 97
};

/*
 * Return the next value from the numbers array. sample keeps an internal
 * index into the array that wraps around when it reaches the end.
 */

static rrd_value_t
sample(void)
{
    rrd_value_t v;
    static size_t i = 0;
    v.int64 = numbers[i++ % (sizeof(numbers) / sizeof(numbers[0]))];
    printf("sample called: %"PRId64"\n", v.int64);
    return v;
}

RRD_SOURCE create_rrd_source(char *name, char *description, rrd_owner_t own, char *owner_uuid,
        char *units, rrd_scale_t scl, rrd_type_t typ, char *min, char *max, int rrd_default, rrd_value_t (*f)())
{
    RRD_SOURCE src;
    src.name = name;
    src.description = description;
    src.owner = own;
    src.owner_uuid = owner_uuid;
    src.rrd_units = units;
    src.scale = scl;
    src.type = typ;
    src.min = min;
    src.max = max;
    src.rrd_default = rrd_default;
    src.sample = (*f);
    return src;
}

int main(int argc, char **argv)
{
    RRD_PLUGIN *plugin1, *plugin2;
    RRD_SOURCE src1[2], src2[2];
    int rc;

    if (argc != 1) {
        fprintf(stderr, "usage: %s\n", basename(argv[0]));
        exit(1);
    }

    /* Tests for plugin1 and sources */
    plugin1 = rrd_open("rrdplugin1", RRD_LOCAL_DOMAIN, "rrdplugin1.rrd");
    assert(plugin1);

    /* Test for adding datasource:RRD_SOURCE_1 to plugin:rrdplugin1 */
    src1[0] = create_rrd_source("RRD_SOURCE_1", "First RRD source", RRD_HOST, "4cc1f2e0-5405-11e6-8c2f-572fc76ac144",
            "BYTE", RRD_GAUGE, RRD_INT64, "-inf", "inf", 1, sample);
    rrd_add_src(plugin1, &src1[0]);
    sleep(1);
    rc = rrd_sample(plugin1);
    assert(rc == RRD_OK);

    /* Call rrd_sample to update value */
    sleep(1);
    rc = rrd_sample(plugin1);
    assert(rc == RRD_OK);

    /* Test for adding datasource:RRD_SOURCE_2 to plugin:rrdplugin1 */
    src1[1] = create_rrd_source("RRD_SOURCE_2", "Second RRD source", RRD_HOST, "e8969702-5414-11e6-8cf5-47824be728c3",
            "BYTE", RRD_GAUGE, RRD_INT64, "-inf", "inf", 1, sample);
    rrd_add_src(plugin1, &src1[1]);
    sleep(1);
    rc = rrd_sample(plugin1);
    assert(rc == RRD_OK);

    /* Call rrd_sample to update value */
    sleep(1);
    rc = rrd_sample(plugin1);
    assert(rc == RRD_OK);

    /* Test for deleting datasource:RRD_SOURCE_1 from plugin:rrdplugin1 */
    rrd_del_src(plugin1, &src1[0]);
    sleep(1);
    rc = rrd_sample(plugin1);
    assert(rc == RRD_OK);

    /* Call rrd_sample to update value */
    sleep(1);
    rc = rrd_sample(plugin1);
    assert(rc == RRD_OK);

    /* Test for deleting datasource:RRD_SOURCE_2 from plugin:rrdplugin1 */
    rrd_del_src(plugin1, &src1[1]);
    sleep(1);
    rc = rrd_sample(plugin1);
    assert(rc == RRD_OK);
    rc = rrd_close(plugin1);
    assert(rc == RRD_OK);

    /* Tests for plugin2 and sources */
    plugin2 = rrd_open("rrdplugin2", RRD_LOCAL_DOMAIN, "rrdplugin2.rrd");
    assert(plugin2);

    /* Test for adding datasource:RRD_SOURCE_1 to plugin:rrdplugin2 */
    src2[0] = create_rrd_source("RRD_SOURCE_1", "First RRD source", RRD_HOST, "ff12b384-96f1-4142-a9c6-21db5fedb4a1",
            "BYTE", RRD_GAUGE, RRD_INT64, "-inf", "inf", 1, sample);
    rrd_add_src(plugin2, &src2[0]);
    sleep(1);
    rc = rrd_sample(plugin2);
    assert(rc == RRD_OK);

    /* Call rrd_sample to update value */
    sleep(1);
    rc = rrd_sample(plugin1);
    assert(rc == RRD_OK);

    /* Test for adding datasource:RRD_SOURCE_2 to plugin:rrdplugin2 */
    src2[1] = create_rrd_source("RRD_SOURCE_2", "Second RRD source", RRD_HOST, "7730f117-5817-4aee-bbcd-4079633ee04a",
            "BYTE", RRD_GAUGE, RRD_INT64, "-inf", "inf", 1, sample);
    rrd_add_src(plugin2, &src2[1]);
    sleep(1);
    rc = rrd_sample(plugin2);
    assert(rc == RRD_OK);

    /* Call rrd_sample to update value */
    sleep(1);
    rc = rrd_sample(plugin2);
    assert(rc == RRD_OK);

    /* Test for deleting datasource:RRD_SOURCE_1 from plugin:rrdplugin2 */
    rrd_del_src(plugin2, &src2[0]);
    sleep(1);
    rc = rrd_sample(plugin2);
    assert(rc == RRD_OK);

    /* Call rrd_sample to update value */
    sleep(1);
    rc = rrd_sample(plugin2);
    assert(rc == RRD_OK);

    /* Test for deleting datasource:RRD_SOURCE_2 from plugin:rrdplugin2 */
    rrd_del_src(plugin2, &src2[1]);
    sleep(1);
    rc = rrd_sample(plugin2);
    assert(rc == RRD_OK);
    rc = rrd_close(plugin2);
    assert(rc == RRD_OK);

    return 0;
}
