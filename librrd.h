
typedef enum { LOCAL_DOMAIN, INTER_DOMAIN } domain;

struct RRD_plugin {
	char *name;		/* name of the plugin */
	domain domain;		/* domain of this plugin */
	float read_frequency;	/* time in seconds */
	float sleep;		/* time in seconds before next action */
};

typedef enum { GAUGE, ABSOLUTE, DERIVE } ty;
typedef enum { HOST, VM, SR } owner;

struct Source {
	char *name;		/* name of the data source */
	char *description;	/* for user interface */
	char *unit;		/* for user interface */
	ty ty;			/* type of value */
	float initial_value;
	float value;
	float min;		/* min <= value <= max */
	float max;		/* min <= value <= max */
	owner owner;
	char* owner_uuid;	/* UUID of the owner or NULL */
};


