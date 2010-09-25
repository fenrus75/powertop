#ifndef _INCLUDE_GUARD_BACKLIGHT_H
#define _INCLUDE_GUARD_BACKLIGHT_H


#include "device.h"

class backlight: public device {
	int min_level, max_level;
	int start_level, end_level;
	char sysfs_path[4096];
	char name[4096];
	int r_index;
	int r_index_power;
public:

	backlight(char *_name, char *path);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "backlight";};

	virtual const char * device_name(void);
	virtual const char * human_name(void) { return "Display backlight";};
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
};

extern void create_all_backlights(void);


#endif