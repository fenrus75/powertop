#ifndef _INCLUDE_GUARD_RFKILL_H
#define _INCLUDE_GUARD_RFKILL_H


#include "device.h"
#include "../parameters/parameters.h"

class rfkill: public device {
	int start_soft, end_soft;
	int start_hard, end_hard;
	char sysfs_path[4096];
	char name[4096];
	char humanname[4096];
	int index;
	int rindex;
public:

	rfkill(char *_name, char *path);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "radio";};

	virtual const char * device_name(void);
	virtual const char * human_name(void) { return humanname; };
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual int power_valid(void) { return utilization_power_valid(rindex);};
};

extern void create_all_rfkills(void);


#endif