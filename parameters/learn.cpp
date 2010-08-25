#include "parameters.h"
#include "../measurement/measurement.h"

#include <stdio.h>


double calculate_params(struct parameter_bundle *params)
{
	unsigned int i;

	params->score = 0;

	for (i = 0; i < past_results.size(); i++) 
		compute_bundle(params, past_results[i]);

	return params->score;
}


/* leaks like a sieve */
void learn_parameters(void)
{
	struct parameter_bundle *best_so_far;
	double best_score = 10000000000000000.0;
        map<string, double>::iterator it;
	int retry = 50;

	double delta = 0.50;

	best_so_far = &all_parameters;

	calculate_params(best_so_far);
	printf("Starting score %5.1f\n", best_so_far->score);
	best_score = best_so_far->score;
	dump_parameter_bundle(best_so_far);


	while (retry--) {
		int changed  = 0;
	        for (it = best_so_far->parameters.begin(); it != best_so_far->parameters.end(); it++) {
			double value, orgvalue;

			orgvalue = value = best_so_far->parameters[it->first];
			if (value == 0.0)
				value = 0.1;
			else
				value = value * (1 + delta);

			if (it->first == "base power" && value > min_power)
				value = min_power;

			printf("Trying %s %5.1f -> %5.1f\n", it->first.c_str(), best_so_far->parameters[it->first], value);
			best_so_far->parameters[it->first] = value;

			calculate_params(best_so_far);
			if (best_so_far->score < best_score) {
				best_score = best_so_far->score;
				orgvalue = value;
				printf("Better score %5.1f\n", best_so_far->score);
				dump_parameter_bundle(best_so_far);
				changed++;
			}

			value = orgvalue * 1 / (1 + delta);


			printf("Trying %s %5.1f -> %5.1f\n", it->first.c_str(), best_so_far->parameters[it->first], value);
			best_so_far->parameters[it->first] = value;

			calculate_params(best_so_far);
			if (best_so_far->score < best_score) {
				best_score = best_so_far->score;
				printf("Better score %5.1f\n", best_so_far->score);
				dump_parameter_bundle(best_so_far);
				changed++;
			} else {
				best_so_far->parameters[it->first] = orgvalue;
			}

		}
		if (!changed)
			delta = delta * 0.8;
	}

	

	calculate_params(best_so_far);
	printf("Final score %5.1f\n", best_so_far->score);
	dump_parameter_bundle(best_so_far);
}