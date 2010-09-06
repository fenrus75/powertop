#include "parameters.h"
#include "../measurement/measurement.h"
#include <stdio.h>
#include <string.h>
#include <vector>

struct parameter_bundle all_parameters;
struct result_bundle all_results;

vector <struct result_bundle *> past_results;


map<string, class device *> devices;

void register_parameter(const char *name, double default_value)
{
	if (all_parameters.parameters[name] <= 0.0001) 
		all_parameters.parameters[name] = default_value;
}

double get_parameter_value(const char *name, struct parameter_bundle *the_bundle)
{
	return the_bundle->parameters[name];
}

double get_result_value(const char *name, struct result_bundle *the_bundle)
{
	return the_bundle->utilization[name];
}


void register_result_device(const char *name, class device *device)
{
	devices[name] = device;
}

int result_device_exists(const char *name)
{
	if (devices.find(name) == devices.end())
		return 0;
	return 1;	
}

void report_utilization(const char *name, double value, struct result_bundle *bundle)
{
	bundle->utilization[name] = value;
}



double compute_bundle(struct parameter_bundle *parameters, struct result_bundle *results)
{
	double power = 0;
	map<string, class device *>::iterator it;
	
	power = parameters->parameters["base power"];

	for (it = devices.begin(); it != devices.end(); it++) {
		class device *device;
		device = it->second;

		power += device->power_usage(results, parameters);
	}

	parameters->actual_power = results->power;
	parameters->guessed_power = power;
	parameters->score += (power - results->power) * (power - results->power);

	return power;
}

double bundle_power(struct parameter_bundle *parameters, struct result_bundle *results)
{
	double power = 0;
	map<string, class device *>::iterator it;
	
	power = parameters->parameters["base power"];

	for (it = devices.begin(); it != devices.end(); it++) {
		class device *device;
		device = it->second;

		power += device->power_usage(results, parameters);
	}

	return power;
}


void dump_parameter_bundle(struct parameter_bundle *para)
{
	map<string, double>::iterator it;

	printf("\n\n");
	printf("Parameter state \n");
	printf("----------------------------------\n");
	printf("Value\t\tName\n");
	for (it = para->parameters.begin(); it != para->parameters.end(); it++) {
		printf("%5.2f\t\t%s\n", it->second, it->first.c_str());
	}

	printf("\n");
	printf("Score:  %5.1f\n", para->score / (0.001 + past_results.size()));
	printf("Guess:  %5.1f\n", para->guessed_power);
	printf("Actual: %5.1f\n", para->actual_power);

	printf("----------------------------------\n");
}

void dump_result_bundle(struct result_bundle *res)
{
	map<string, double>::iterator it;

	printf("\n\n");
	printf("Utilisation state \n");
	printf("----------------------------------\n");
	printf("Value\t\tName\n");
	for (it = res->utilization.begin(); it != res->utilization.end(); it++) {
		printf("%5.2f%%\t\t%s\n", it->second, it->first.c_str());
	}

	printf("\n");
	printf("Power: %5.1f\n", res->power);

	printf("----------------------------------\n");
}

struct result_bundle * clone_results(struct result_bundle *bundle)
{
	struct result_bundle *b2;
	map<string, double>::iterator it;

	b2 = new struct result_bundle;

	if (!b2)
		return NULL;

	b2->power = bundle->power;

	for (it = bundle->utilization.begin(); it != bundle->utilization.end(); it++) {
		b2->utilization[it->first] = it->second;
	}

	return b2;
}


struct parameter_bundle * clone_parameters(struct parameter_bundle *bundle)
{
	struct parameter_bundle *b2;
	map<string, double>::iterator it;

	b2 = new struct parameter_bundle;

	if (!b2)
		return NULL;

	b2->score = 0;
	b2->guessed_power = 0;
	b2->actual_power = bundle->actual_power;

	for (it = bundle->parameters.begin(); it != bundle->parameters.end(); it++) {
		b2->parameters[it->first] = it->second;
	}

	return b2;
}


void store_results(void)
{
	global_joules_consumed();
	if (all_results.power > 0.01)
		past_results.push_back(clone_results(&all_results));	
}



void dump_past_results(void)
{
	unsigned int j;
	unsigned int i;
	struct result_bundle *result;

	for (j = 0; j < past_results.size(); j+=10) {
		printf("Est    ");
		for (i = j; i < past_results.size() && i < j + 10; i++) {
			result = past_results[i];
			printf("%6.2f  ", bundle_power(&all_parameters, result));
		}
		printf("\n");
		printf("Actual ");
		for (i = j; i < past_results.size() && i < j + 10; i++) {
			result = past_results[i];
			printf("%6.2f  ", result->power);
		}
		printf("\n\n");
	}
}