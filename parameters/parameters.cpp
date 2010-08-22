#include "parameters.h"
#include <stdio.h>

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
	
	power = parameters->parameters["base power"];

	for (it = results->utilization.begin(); it != results->utilization.end(); it++) {
		class device *device;
		device = devices[it->first];

		power += device->power_usage(it->second, parameters);
	}

	parameters->actual_power = results->power;
	parameters->guessed_power = power;
	parameters->score = (power - results->power) * (power - results->power);
}


void dump_parameter_bundle(struct parameter_bundle *para)
{
	map<const char *, double>::iterator it;

	printf("\n\n");
	printf("Parameter state \n");
	printf("----------------------------------\n");
	printf("Value\t\tName\n");
	for (it = para->parameters.begin(); it != para->parameters.end(); it++) {
		printf("%5.2f\t\t%s\n", it->second, it->first);
	}

	printf("\n");
	printf("Score:  %5.1f\n", para->score);
	printf("Guess:  %5.1f\n", para->guessed_power);
	printf("Actual: %5.1f\n", para->actual_power);

	printf("----------------------------------\n");
}