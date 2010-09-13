#include "../parameters/parameters.h"
#include <stdio.h>
#include <stdlib.h>

int debug_learning = 0;
static double compute_bundle(struct parameter_bundle *parameters, struct result_bundle *results, char *except)
{
	double power = 0;
	unsigned int i;
	
	power = parameters->parameters["base power"];

	for (i = 0; i < all_devices.size(); i++) {
		if (strcmp(all_devices[i]->device_name(), except) == 0)
			continue;
		power += all_devices[i]->power_usage(results, parameters);
	}

	parameters->actual_power = results->power;
	parameters->guessed_power = power;

	return power;
}

static double compute_device_power(struct parameter_bundle *parameters, struct result_bundle *results, char *except)
{
	double power = 0;
	unsigned int i;
	

	for (i = 0; i < all_devices.size(); i++) {
		if (strcmp(all_devices[i]->device_name(), except) != 0)
			continue;
		power += all_devices[i]->power_usage(results, parameters);
	}

	return power;
}

int main(int argc, char **argv)
{
	char except_device[4096], param[4096];
	unsigned int i;
	double org;

	except_device[0] = 0;
	param[0] = 0;
	create_all_devices();
	load_results("saved_results.powertop");
	load_parameters("saved_parameters.powertop");
	
	if (argc < 3) {
		map<string, double>::iterator it;

		printf("\tdevice names:\n");
		printf("usage:\n\tgraphparameter <device name> <parameter name>\n");
		printf("\n\tdevice names:\n");
		for (i = 0; i < all_devices.size(); i++) {
			printf("\t%s\n", all_devices[i]->device_name());
		}
		printf("\n\tparameter names:\n");

		for (it = all_parameters.parameters.begin(); it != all_parameters.parameters.end(); it++) {
			printf("\t%s\n", it->first.c_str());
		}
		exit(0);
	}

	strcpy(except_device, argv[1]);
	strcpy(param, argv[2]);


	org = all_parameters.parameters[param];
	all_parameters.parameters[param] = 0;
	learn_parameters(5, param);
	learn_parameters(100, param);
	learn_parameters(200, param);
	learn_parameters(300, param);
	if (debug_learning)
		dump_parameter_bundle();
	all_parameters.parameters[param] = org;

	printf("Parameter\testimated\tsystem delta\n");
	for (i = 0; i < past_results.size(); i++) {
		struct result_bundle *results = past_results[i];
		double delta, devpower;
		double par;

		delta = results->power - compute_bundle(&all_parameters, results, except_device);
		devpower = compute_device_power(&all_parameters, results, except_device);
		par = results->utilization[param];

		
		printf("%5.2f\t\t%5.2f\n",
			par, delta);
	}

	return 0;
}