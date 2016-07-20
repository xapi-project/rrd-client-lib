/*
 * Copyright (C) Citrix Systems Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2.1 only
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/time.h>

#define DEFAULT_HEADER              "DATASOURCES"
#define DATA_CRC_BYTES              4
#define METADATA_CRC_BYTES          4
#define DATASOURCE_COUNT_BYTES      4
#define TIMESTAMP_BYTES             8
#define DATASOURCE_VALUE_BYTES      8
#define METADATA_LENGTH_BYTES       4

/* Define a plugin */
struct RRD_Plugin {
	/* name of the plugin */
	char plugin_name[100];
	/* plugin domain 'Local' or 'Interdomain' */
	char plugin_domain[100];
	/*
	 * The time period in seconds when RRD daemon collects a new batch of data
	 * Currently the supported value is 5 seconds
	 */
	int read_frequency;
	/*
	 * Time in seconds for wake_up_before_next_reading() to return before
	 * the next RRDD reading is performed
	 */
	float time_to_reading;
};

/* Define a Datasource */
struct Datasource {
	/* The Datasource name */
	char name[100];
	/* Starting value  */
	float value;
	/* Value type - Will try to convert the values into string or int or vice-versa */
	char value_type[100];
	/* 
	 * Datasource's description; will be displayed in XenCenter
	 * Default: (the empty string)
	 */
	char description[100];
	/* Datasource type: 'gauge', 'absolute' or 'derive' - Default: 'gauge' */
	char type[100];
	/* Minimum value the Datasource can take - Default: float('-inf') */
	float min;
	/* Maximum value the Datasource can take - Default: float('inf') */
	float max;
	/* The unit the Datasource is counted in - Default: (the empty string) */
	char units[100];
	/* RRDs belong to object 'Host', 'VM <vm-uuid>', 'SR <sr-uuid>' - Default: 'Host' */
	char owner[100];
};

/* Functions for RRDD plugins */

/* Get the absolute path to the memory mapped file for RRDDs */
char *get_path(void);

/*
 * Writes Datasource object dict to memory mapped file.
 * MUST be called the first time and every time there is a change in the dict
 * (Apart from the `Datasources values)
 */
void full_update(struct Datasource data_source);

/* Updates `Datasources values in memory mapped file. */
void fast_update(struct Datasource data_source);

/*
 * Blocks until RRD_plugin.time_to_reading seconds before the next
 * reading of the memory mapped file by the RRDD
 */
void wake_up_before_next_reading()

/* Functions for Datasources */

/* Datasource properties are same as defined in Datasource struct */
void get_property(struct Datasource data_source);

/* Change the Datasource 'value' property */
void set_value(float value);

/* Struct to define a RRD plugin process */
struct Process {
	char name[100];
};

/* Initialise the process for registering the plugin */
void initialise (struct Process process);

/* Start writing the RRDs based on plugin domain: `local or `interdomain */
void start_local (struct Process process, float neg_shift, int target, void *dss_f());

/* Payload struct used for read or write */
struct payload{
	struct Datasource datasource;
	float timestamp;
};

/* Function for writing the payload */
void write_payload (struct Payload payload);

/* Function for reading the payload */
struct Payload read_payload ();

/* Need to define functions for Read and Write */
/* 
 * header
 * data_crc
 * metadata_crc
 * datasource_count
 * timestamp
 * datasource_values
 * metadata_length
 * metadata
 */

