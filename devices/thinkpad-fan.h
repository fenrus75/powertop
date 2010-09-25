#ifndef _INCLUDE_GUARD_THINKPAD_FAN_H
#define _INCLUDE_GUARD_THINKPAD_FAN_H


#include "device.h"

class thinkpad_fan: public device {
	double start_rate, end_rate;
public:

	thinkpad_fan();

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "fan";};

	virtual const char * device_name(void) { return "Fan-1";};
	virtual const char * human_name(void) { return "Laptop fan";};
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
};

extern void create_thinkpad_fan(void);


#endif