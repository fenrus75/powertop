#include "parameters.h"
#include "../measurement/measurement.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


double calculate_params(struct parameter_bundle *params)
{
	unsigned int i;

	params->score = 0;

	for (i = 0; i < past_results.size(); i++) 
		compute_bundle(params, past_results[i]);

	return params->score;
}


/* 
 * gradual linear convergence of non-independent variables works better if once in a while
 * you make a wrong move....
 */
static int random_disturb(int retry_left)
{
	if (retry_left < 10)
		return 0;
	
	if ( (rand() % 100) == 7)
		return 1;
	return 0;
}

static unsigned int previous_measurements;

/* leaks like a sieve */
void learn_parameters(int iterations)
{
	struct parameter_bundle *best_so_far;
	double best_score = 10000000000000000.0;
        map<string, double>::iterator it;
	int retry = iterations;

	if (past_results.size() == previous_measurements)
		return;

	previous_measurements = past_results.size();

	double delta = 0.50;

	best_so_far = &all_parameters;

	calculate_params(best_so_far);
	best_score = best_so_far->score;

	delta = 0.001 / pow(0.8, iterations / 2.0);
	if (iterations < 25)
		delta = 0.001 / pow(0.5, iterations / 2.0);

	if (delta > 0.2)
		delta = 0.2;

//	printf("Delta starts at %5.3f\n", delta);

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

			if (it->first == "base power" && orgvalue > min_power)
				orgvalue = min_power;

			if (value > 5000)
				value = 5000;

//			printf("Trying %s %5.1f -> %5.1f\n", it->first.c_str(), best_so_far->parameters[it->first], value);
			best_so_far->parameters[it->first] = value;

			calculate_params(best_so_far);
			if (best_so_far->score < best_score || random_disturb(retry)) {
				best_score = best_so_far->score;
				orgvalue = value;
//				printf("Better score %5.1f\n", best_so_far->score);
//				dump_parameter_bundle(best_so_far);
				changed++;
			}

			value = orgvalue * 1 / (1 + delta);


			if (value > 5000)
				value = 5000;


//			printf("Trying %s %5.1f -> %5.1f\n", it->first.c_str(), orgvalue, value);
			best_so_far->parameters[it->first] = value;

			calculate_params(best_so_far);
			if (best_so_far->score < best_score || random_disturb(retry)) {
				best_score = best_so_far->score;
//				printf("Better score %5.1f\n", best_so_far->score);
//				dump_parameter_bundle(best_so_far);
				changed++;
			} else {
				best_so_far->parameters[it->first] = orgvalue;
			}

		}
		if (!changed) {
			double mult;
			mult = 0.8;
			if (iterations < 25)
				mult = 0.5;
			delta = delta * mult;
		}

		if (delta < 0.001)
			break;
	}

	

	calculate_params(best_so_far);
	printf("Final score %5.1f (%i points)\n", best_so_far->score / past_results.size(), past_results.size());
//	dump_parameter_bundle(best_so_far);
//	dump_past_results();
}