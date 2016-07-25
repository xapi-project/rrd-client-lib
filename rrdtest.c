/* vim: set ts=8 sw=8 noet: */

#include <stdio.h>
#include <stdlib.h>
#include "parson/parson.h"

int main(int argc, char** argv)
{
  char *buf = NULL;
  size_t buf_size;

  JSON_Value *root_val = json_value_init_object();
  JSON_Object *root = json_value_get_object(root_val);

  JSON_Value *src_val = json_value_init_object();
  JSON_Object *src = json_value_get_object(src_val);
  json_object_set_string(src, "description", "The current time");
  json_object_set_string(src, "owner", "host");
  json_object_set_string(src, "value_type", "int64");
  json_object_set_boolean(src, "default", 1);
  json_object_set_string(src, "units", "seconds");
  json_object_set_string(src, "min", "-inf");
  json_object_set_string(src, "max", "inf");
  json_object_set_value(root, "datasources", src_val);

  buf_size = json_serialization_size_pretty(root_val);
  buf = malloc(buf_size);
  if (!buf || !buf_size) {
	  perror("can't allocate memory for JSON");
	  exit(1);
  }
  json_serialize_to_buffer_pretty(root_val, buf, buf_size);
  puts(buf);
  free(buf);

  json_value_free(root_val);
  return 0;
}
