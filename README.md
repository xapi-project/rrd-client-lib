
# librdd - A Library to Provide RRD Data

The RRD services is a system-level service that aggregates
quasi-continuous numerical data. For example, it can aggregate data like
CPU load or disk usage.  Unlike syslog, RRD aggregates the data: an
RRD plugin reports data every 5 seconds but the service aggregates older
data into statistical values (like minimum, maximum, and mean values.)
This library provides functions to register a plugin with the RRD
service and to provide data sources. All data provided by the plugin is
written periodically to a file from where it is read by the RRD daemon.

    <<librrd.h>>=
    #include <stdint.h>
    <<constants>>
    <<type definitions>>
    <<function declarations>>
    

## Open, Sample, Close

A plugin opens a file for communication and periodically calls
``rrd_sample`` to provide data. The type `RRD_PLUGIN` should be
considered private to the library.

    <<type definitions>>=
    typedef enum { LOCAL_DOMAIN, INTER_DOMAIN } rrd_domain;
    
    
    <<function declarations>>=
    RRD_PLUGIN *rrd_open(char *name, rrd_domain domain, char *path);
    int rrd_sample(RRD_PLUGIN *plugin); /* call periodically */
    int rrd_close(RRD_PLUGIN *plugin);
    

The name of the plugin is descriptive, as whether it reports data
for a single machine (`LOCAL_DOMAIN`) or multiple (`INTER_DOMAIN`).


## Data Sources

A typical client has several data sources. A data source either reports
integer or float values. Data sources can be added and removed
dynamically.

    <<type definitions>>=
    typedef enum { GAUGE, ABSOLUTE, DERIVE } scale;
    typedef enum { HOST, VM, SR } owner;
    typedef enum { FLOAT64, INT64 } type;
    
    typedef union {
            float   float64;
            int64_t int64;
    } rrd_value;
    
    typedef struct rrd_source {
            char *name;                     /* name of the data source */
            char *description;              /* for user interface */
            owner owner;
            char *owner_uuid;               /* UUID of the owner or NULL */
            char *units;                    /* for user interface */
            scale scale ;                   /* presentation of value */
            type type;                      /* type of value */
            rrd_value min;                  /* min <= sample() <= max */
            rrd_value max;                  /* min <= sample() <= max */
            rrd_value (*sample)(void);      /* reads value that gets reported */
    } RRD_SOURCE;
    
    <<typedef RRD_PLUGIN>>
    
    <<function declarations>>=
    int rrd_add_src(RRD_PLUGIN *plugin, RRD_SOURCE *source);
    int rrd_del_src(RRD_PLUGIN *plugin, RRD_SOURCE *source);
    
    
An `RRD_SOURCE` has several descriptive fields for the value it is
reporting. It includes a function (pointer) `sample` that obtains the
value being reported. A value can be either a 64-bit integer or a 64-bit
floating point value -- discriminated by `type`.

## Error Handling

Some functions return an error code.

    <<constants>>=
    #define RDD_OK                    0
    #define RRD_TOO_MANY_SOURCES      1
    #define RRD_NO_SOUCH_SOURCE       2
    #define RRD_ERROR                 3
    

## Private Types

    <<constants>>=
    #define RRD_MAX_SOURCES           128
    
    
    <<typedef RRD_PLUGIN>>=
    typedef struct rrd_plugin {
            char *name;                     /* name of the plugin */
            rrd_domain domain;              /* domain of this plugin */
            RRD_SOURCE sources[RRD_MAX_SOURCES];
            int dirty;                      /* true if sources changed */
    	char *json;			/* meta data */
    } RRD_PLUGIN;
    

