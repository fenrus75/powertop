#ifndef _INCLUDE_GUARD_i915_GPU_H
#define _INCLUDE_GUARD_i915_GPU_H


#include "device.h"

class i915gpu: public device {
	int index;
	int rindex;
public:

	i915gpu();

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "GPU";};

	virtual const char * device_name(void) { return "GPU";};
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual bool show_in_list(void) {return false;};
	virtual const char * util_units(void) { return " ops/s"; };
};

extern void create_i915_gpu(void);


#endif