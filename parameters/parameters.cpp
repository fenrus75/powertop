#include "parameters.h"

struct parameter_bundle all_parameters;
struct result_bundle all_results;

map<const char *, class device *> devices;

void register_parameter(const char *name, double default_value)
{
	all_parameters.parameters[name] = default_value;
}

double get_parameter_value(const char *name, struct parameter_bundle &the_bundle)
{
	return the_bundle.parameters[name];
}


void register_result_device(const char *name, class device *device)
{
	devices[name] = device;
}
