
#define RRD_MAX_SOURCES		  128

#define RDD_OK				  0
#define RRD_TOO_MANY_SOURCES  1
#define RRD_NO_SOUCH_SOURCE	  2
#define RRD_ERROR			  3

typedef enum { LOCAL_DOMAIN, INTER_DOMAIN } rrd_domain;
typedef enum { GAUGE, ABSOLUTE, DERIVE } kind;
typedef enum { HOST, VM, SR } owner;

typedef struct rrd_source {
	char *name;				/* name of the data source */
	char *description;		/* for user interface */
	owner owner;
	char *owner_uuid;		/* UUID of the owner or NULL */
	char *units;			/* for user interface */
	kind value_type;		/* type of value */
	float min;				/* min <= sample() <= max */
	float max;				/* min <= sample() <= max */
	float (*sample)();		/* reads value that gets reported */
} RRD_SOURCE;

/* the type struct rrd_plugin below is private to the implementation and
 * entirely managed by it.
 */

typedef struct rrd_plugin {
	char *name;				/* name of the plugin */
	rrd_domain domain;		/* domain of this plugin */
	RRD_SOURCE sources[RRD_MAX_SOURCES];
	int dirty;				/* true if sources changed since last sample */
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




