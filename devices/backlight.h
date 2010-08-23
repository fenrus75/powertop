#ifndef _INCLUDE_GUARD_BACKLIGHT_H
#define _INCLUDE_GUARD_BACKLIGHT_H


#include "device.h"

class backlight: public device {
	int min_level, max_level;
	int start_level, end_level;
	char sysfs_path[4096];
public:

	backlight(char *path);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "backlight";};

};

extern void create_all_backlights(void);


#endif