
#define RRD_MAX_SOURCES		  128

#define RDD_OK			  0
#define RRD_TOO_MANY_SOURCES	  1
#define RRD_NO_SOUCH_SOURCE	  2
#define RRD_ERROR		  3

typedef enum { LOCAL_DOMAIN, INTER_DOMAIN } rrd_domain;
typedef enum { GAUGE, ABSOLUTE, DERIVE } scale;
typedef enum { HOST, VM, SR } owner;
typedef enum { FLOAT64, INT64 } type;

/* An RRD_SOURCE can report float or integer values - represented as
 * a rrd_value. The type component tells which one it is.
 */
typedef union {
	float   float64;
	int	int64;
} rrd_value;

/* An RRD_SOURCE describes the value being reported and contains a
 * sample() function to obtain such values.
 */
typedef struct rrd_source {
	char *name;			/* name of the data source */
	char *description;		/* for user interface */
	owner owner;
	char *owner_uuid;		/* UUID of the owner or NULL */
	char *units;			/* for user interface */
	scale scale ;			/* presentation of value */
	type type;			/* type of value */
	rrd_value min;			/* min <= sample() <= max */
	rrd_value max;			/* min <= sample() <= max */
	rrd_value (*sample)();		/* reads value that gets reported */
} RRD_SOURCE;

/* The RRD_PLUGIN below is private to the implementation and
 * entirely managed by it.
 */
typedef struct rrd_plugin {
	char *name;			/* name of the plugin */
	rrd_domain domain;		/* domain of this plugin */
	RRD_SOURCE sources[RRD_MAX_SOURCES];
	int dirty;			/* true if sources changed */
} RRD_PLUGIN;


/*
 * rrd_open - regsiter a new plugin
 * name: name of the plugin
 * domain: INTER_DOMAIN if it reports data from multiple domains
 * path: file path where the plugin writes it samples
 * returns:
 * NULL on error
 */
RRD_PLUGIN *rrd_open(char *name, rrd_domain domain, char *path);

/*
 * rrd_close - close a plugin
 */
int rrd_close(RRD_PLUGIN *plugin);

/* rrd_add_src - add a new data source
 * returns: error code
 * At most RRD_MAX_SOURCES can be active. It is an unchecked error to
 * register the same source multiple times.
 */
int rrd_add_src(RRD_PLUGIN *plugin, RRD_SOURCE *source);

/* rrd_del_src - remove a data source
 * returns: error code
 */
int rrd_del_src(RRD_PLUGIN *plugin, RRD_SOURCE *source);

/* calling rrd_sample(plugin) triggers that all data sources are
 * sampled and the results are reported to the RRD daemon. This
 * function needs to be called every 5 seconds by the client,
 */
int rrd_sample(RRD_PLUGIN *plugin);




