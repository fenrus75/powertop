#ifndef _INCLUDE_GUARD_ALSA_H
#define _INCLUDE_GUARD_ALSA_H


#include "device.h"
#include "../parameters/parameters.h"

#include <stdint.h>

class alsa: public device {
	uint64_t start_active, end_active;
	uint64_t start_inactive, end_inactive;
	char sysfs_path[4096];
	char name[4096];
	char humanname[4096];
	int rindex;
public:

	alsa(char *_name, char *path);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "alsa";};

	virtual const char * device_name(void);
	virtual const char * human_name(void) { return humanname; };
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual int power_valid(void) { return utilization_power_valid(rindex);};

};

extern void create_all_alsa(void);


#endif