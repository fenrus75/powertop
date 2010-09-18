#include "parameters.h"
#include "../measurement/measurement.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>

struct parameter_bundle all_parameters;
struct result_bundle all_results;

vector <struct result_bundle *> past_results;

map <string, int> param_index;
static int maxindex = 0;


void register_parameter(const char *name, double default_value)
{
	int index;

	index = param_index[name];
	if (index == 0) {
		index = param_index[name] = ++maxindex;
	}

	if (index >= (int)all_parameters.parameters.size())
		all_parameters.parameters.resize(index+1);

	if (all_parameters.parameters[index] <= 0.0001) 
		all_parameters.parameters[index] = default_value;
}

void set_parameter_value(const char *name, double value, struct parameter_bundle *bundle)
{
	int index;

	index = param_index[name];
	if (index == 0) {
		index = param_index[name] = ++maxindex;
	}

	if (index >= (int)bundle->parameters.size())
		bundle->parameters.resize(index+1);

	bundle->parameters[index] = value;
}

double get_parameter_value(const char *name, struct parameter_bundle *the_bundle)
{
	int index;

	index = param_index[name];
	if (index == 0) {
		index = param_index[name] = ++maxindex;
	}

	return the_bundle->parameters[index];
}

double get_result_value(const char *name, struct result_bundle *the_bundle)
{
	return the_bundle->utilization[name];
}


int result_device_exists(const char *name)
{
	unsigned int i;
	for (i = 0; i < all_devices.size(); i++) {
		if (strcmp(all_devices[i]->device_name(), name) == 0)
			return 1;
	}
	return 0;
}

void report_utilization(const char *name, double value, struct result_bundle *bundle)
{
	bundle->utilization[name] = value;
}



double compute_bundle(struct parameter_bundle *parameters, struct result_bundle *results)
{
	double power = 0;
	unsigned int i;

	static int bpi = 0;

	if (!bpi)
		bpi = param_index["base power"];
	
	power = parameters->parameters[bpi];

	for (i = 0; i < all_devices.size(); i++) {

		power += all_devices[i]->power_usage(results, parameters);
	}
//	printf("result power is %6.2f  guessed is %6.2f\n", results->power, power);
	parameters->actual_power = results->power;
	parameters->guessed_power = power;
	/* scale the squared error by the actual power so that non-idle data points weigh heavier */
	parameters->score += results->power * (power - results->power) * (power - results->power);

	return power;
}

double bundle_power(struct parameter_bundle *parameters, struct result_bundle *results)
{
	double power = 0;
	unsigned int i;
	static int bpi = 0;

	if (!bpi)
		bpi = param_index["base power"];

	
	power = parameters->parameters[bpi];

	for (i = 0; i < all_devices.size(); i++) {

		power += all_devices[i]->power_usage(results, parameters);
	}

	return power;
}


void dump_parameter_bundle(struct parameter_bundle *para)
{
	map<string, int>::iterator it;
	int index;

	printf("\n\n");
	printf("Parameter state \n");
	printf("----------------------------------\n");
	printf("Value\t\tName\n");
	for (it = param_index.begin(); it != param_index.end(); it++) {
		index = it->second;
		printf("%5.2f\t\t%s\n", para->parameters[index], it->first.c_str());
	}

	printf("\n");
	printf("Score:  %5.1f\n", sqrt(para->score / (0.001 + past_results.size()) / average_power()));
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
	unsigned int i;

	b2 = new struct parameter_bundle;

	if (!b2)
		return NULL;

	b2->score = 0;
	b2->guessed_power = 0;
	b2->actual_power = bundle->actual_power;
	b2->parameters.resize(bundle->parameters.size());
	for (i = 0; i < bundle->parameters.size(); i++) {
		b2->parameters[i] = bundle->parameters[i];
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

double average_power(void)
{
	double sum = 0.0;
	unsigned int i;
	for (i = 0; i < past_results.size(); i++)
		sum += past_results[i]->power;

	if (past_results.size()) 
		sum = sum / past_results.size() + 0.0001;
	else
		sum = 0.0001;
	return sum;
}