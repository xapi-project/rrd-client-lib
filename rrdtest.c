/*
 *
 * vim: set ts=4 sw=4 et: 
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <assert.h>

#include "librrd.h"
#include "parson/parson.h"

void
read_src(JSON_Value * root)
{
    JSON_Object    *ds;
    JSON_Value     *d_;
    JSON_Object    *d;
    const char     *name;

    assert(json_value_get_type(root) == JSONObject);
    ds = json_object_get_object(json_value_get_object(root),
                                "datasources");
    assert(ds);
    assert(json_object_get_count(ds) > 0);

    name = json_object_get_name(ds, 0);
    d_ = json_object_get_value_at(ds, 0);
    assert(json_value_get_type(d_) == JSONObject);
    d = json_value_get_object(d_);
    assert(json_object_has_value_of_type(d, "description", JSONString));
    assert(json_object_has_value_of_type(d, "owner", JSONString));
    assert(json_object_has_value_of_type(d, "value_type", JSONString));
    assert(json_object_has_value_of_type(d, "type", JSONString));
    assert(json_object_has_value_of_type(d, "default", JSONBoolean));
    assert(json_object_has_value_of_type(d, "units", JSONString));
    assert(json_object_has_value_of_type(d, "min", JSONString));
    assert(json_object_has_value_of_type(d, "max", JSONString));
    assert(json_object_has_value_of_type(d, "values", JSONArray));

    printf("name = %s\n", name);
}

int
main(int argc, char **argv)
{
    JSON_Value     *test = NULL;
    char           *buf = NULL;
    RRD_PLUGIN     *plugin;

    if (argc != 2) {
        fprintf(stderr, "usage: %s file.json\n", basename(argv[0]));
        exit(1);
    }
    test = json_parse_file(argv[1]);
    if (!test) {
        fprintf(stderr, "can't parse %s\n as a JSON file", argv[1]);
        exit(1);
    }
    buf = json_serialize_to_string_pretty(test);
    puts(buf);
    json_free_serialized_string(buf);

    read_src(test);

    plugin = rrd_open("rrdtest", LOCAL_DOMAIN, "rrdtest.rrd");
    assert(plugin);
    rrd_close(plugin);


    json_value_free(test);
    return 0;
}
