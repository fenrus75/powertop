#ifndef __INCLUDE_GUARD_PARAMETERS_H_
#define __INCLUDE_GUARD_PARAMETERS_H_


#include <map>
#include "../devices/device.h"

using namespace std;

struct parameter_bundle
{
	double score;
	double guessed_power;
	double actual_power;

	double power_offset;
	map<const char *, double> parameters;  /* parameter name, parameter value */
};

extern struct parameter_bundle all_parameters;


extern void register_parameter(const char *name, double default_value = 0.0);
extern double get_parameter_value(const char *name, struct parameter_bundle *bundle = &all_parameters);


struct result_bundle
{
	double power;
	map <const char *, double> utilization; /* device name, device utilization %age */
};

extern struct result_bundle all_results;

extern map<const char *, class device *> devices;

extern void register_result_device(const char *name, class device *device);

extern void report_utilization(const char *name, double value, struct result_bundle *bundle = &all_results);


extern void compute_bundle(struct parameter_bundle *parameters, struct result_bundle *results);

#endif