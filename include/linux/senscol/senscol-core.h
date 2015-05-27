#ifndef	SENSCOL_CORE__
#define	SENSCOL_CORE__

#include <linux/types.h>
#include <linux/kobject.h>

struct data_field;
struct sens_property;
struct senscol_impl;

struct sensor_def {
	char	*name; 				/* Must be not NULL */
	uint32_t	usage_id; 		/* Usage ID */
	char	*friendly_name;			/* May be NULL */
	int	num_data_fields;		/* Size of array of data fields */
	struct data_field *data_fields;		/* Array of data fields */
	int	num_properties;			/* Size of array of properties */
	struct sens_property *properties;	/* Array of properties*/
	uint32_t	id;			/* Unique ID of sensor (running count of discovery iteration) */
	int	sample_size;			/* Derived from array of data_fields, updated when every data_field is added */
	struct senscol_impl	*impl;
	struct list_head	link;
	struct kobject	kobj;
	struct kobject	data_fields_kobj;
	struct kobject	props_kobj;
};

struct data_field {
	char *name;				/* Must be not NULL */
	uint32_t	usage_id;		/* Usage ID of data_field */
	uint8_t	exp;				/* Exponent: 0..F */
	uint8_t	len;				/* Length: 0..4 */
	uint32_t	unit;			/* Usage ID of unit */
	int	is_numeric;			/* If !is_numeric, only name and usage_id appear */
	struct kobject	kobj;
	struct sensor_def	*sensor;	/* We need backlink for properties to their parent sensors */
	int	index;				/* Index of field in raw data */
};

struct sens_property {
	char *name;				/* Must be not NULL */
	uint32_t	usage_id;		/* Usage ID of sens_property */
	char	*value;
	uint32_t	unit;			/* Usage ID of unit */
	int	is_numeric;			/* If !is_numeric, only name and usage_id appear */
	struct kobject	kobj;
	struct sensor_def	*sensor;	/* We need backlink for properties to their parent sensors */
};

/* Only allocates new sensor */
struct sensor_def *alloc_senscol_sensor(void);

/* Init sensor (don't call for initialized sensors */
void	init_senscol_sensor(struct sensor_def *sensor);

/* Exposed sensor via sysfs, structure may be static */
int	add_senscol_sensor(struct sensor_def *sensor);

struct sensor_def	*get_senscol_sensor_by_name(const char *name);
struct sensor_def	*get_senscol_sensor_by_id(uint32_t id);

/* Add data field to existing sensor */
int	add_data_field(struct sensor_def *sensor, struct data_field *data);

/* Add property to existing sensor */
int	add_sens_property(struct sensor_def *sensor, struct sens_property *prop);

/* Get known name of given usages (NULL if unknown) */
const char	*senscol_usage_to_name(unsigned usage);

/* Push data sample in upstream buffer towards user-mode. Sample's size is determined from the structure */
int	push_sample(uint32_t id, void *sample);

/* Get known name of given modifier  safe, always returns value*/
const char	*senscol_get_modifier(unsigned modif);

/* Sample structure. Understood by binary SysFS provider and user-mode client */
struct senscol_sample {
	uint32_t	id;
	uint32_t	size;	/* For easier/faster traversing of FIFO during reads */
	uint8_t	data[1];	/* `size' (one or more) bytes of data */
} __attribute__((packed));

/*
 * Samples are queued is a simple FIFO binary buffer with head and tail pointers.
 * If the buffer wraps around, a single sample not start past SENSCOL_DATA_BUF_LAST, but may cross it or start at it
 * Additional fields if wanted to be communicated to user mode can be define
 */


/*
 * Suggested size of data buffer:
 *   avg 24 bytes per sampl
 *   expected 2600 samples/s for 17 sensors at max. rate
 *   cover for 10 seconds of data
 */
/*#define	SENSCOL_DATA_BUF_SIZE	(24*2600*10)*/
#define	SENSCOL_DATA_BUF_SIZE	(24*2600)
#define	SENSCOL_DATA_BUF_LAST	(SENSCOL_DATA_BUF_SIZE-128)

/*
 * Sensor collection underlying handler.
 * Supplies set_prop(), get_prop() and get_sample() callback
 */
struct senscol_impl {
	/* Get property value, will return NULL on failure */
	int	(*get_sens_property)(struct sensor_def *sensor, const struct sens_property *prop, char *value, size_t val_buf_size);

	/* Set property value */
	int	(*set_sens_property)(struct sensor_def *sensor, const struct sens_property *prop, const char *value);

	/* Get sample */
	int	(*get_sample)(struct sensor_def *sensor, void *sample_buf, size_t sample_buf_size);
	struct list_head link;
};

int	add_senscol_impl(struct senscol_impl *impl);
int	remove_senscol_impl(struct senscol_impl *impl);

#endif /*SENSCOL_CORE__H*/

