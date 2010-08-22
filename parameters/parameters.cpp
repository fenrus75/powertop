#include "parameters.h"

struct parameter_bundle all_parameters;
struct result_bundle all_results;

map<const char *, class device *> devices;

void register_parameter(const char *name, double default_value)
{
	all_parameters.parameters[name] = default_value;
}

double get_parameter_value(const char *name, struct parameter_bundle *the_bundle)
{
	return the_bundle->parameters[name];
}


void register_result_device(const char *name, class device *device)
{
	devices[name] = device;
}

void report_utilization(const char *name, double value, struct result_bundle *bundle)
{
	bundle->utilization[name] = value;
}



void compute_bundle(struct parameter_bundle *parameters, struct result_bundle *results)
{
	double power = 0;
	map<const char *, double>::iterator it;
	
	power = parameters->power_offset;

	for (it = results->utilization.begin(); it != results->utilization.end(); it++) {
		class device *device;
		device = devices[it->first];

		power += device->power_usage(it->second, parameters);
	}

	parameters->actual_power = results->power;
	parameters->guessed_power = power;
	parameters->score = (power - results->power) * (power - results->power);
}