#include "parameters.h"
#include "../measurement/measurement.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


double calculate_params(struct parameter_bundle *params, int last_only)
{
	unsigned int i;
	int from;

	params->score = 0;

	from = 0;
	if (last_only) {
		compute_bundle(params, past_results[0]);
		from = past_results.size() - last_only;
	}

	for (i = from; i < past_results.size(); i++) 
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
void learn_parameters(int iterations, const char *pattern, int last_only)
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

	calculate_params(best_so_far, 0);
	best_score = best_so_far->score;

	delta = 0.001 / pow(0.8, iterations / 2.0);
	if (iterations < 25)
		delta = 0.001 / pow(0.5, iterations / 2.0);

	if (delta > 0.2)
		delta = 0.2;

	if (1.0 * best_score / past_results.size() < 4 && delta > 0.05)
		delta = 0.05;

	printf("Delta starts at %5.3f\n", delta);

	if (best_so_far->parameters["base power"] > min_power)
		best_so_far->parameters["base power"] = min_power;

	best_so_far->parameters["base power"] = best_so_far->parameters["base power"] * 0.99;

	while (retry--) {
		int changed  = 0;
		string bestparam;
		double newvalue;

		bestparam = "";

		calculate_params(best_so_far, last_only);
		best_score = best_so_far->score;

		
	        for (it = best_so_far->parameters.begin(); it != best_so_far->parameters.end(); it++) {
			double value, orgvalue;

			if (pattern) {
				if (strstr(it->first.c_str(), pattern) == NULL && it->first != "base power")
					continue;
				printf("matching %s to %s \n", it->first.c_str(), pattern);
			}

			orgvalue = value = best_so_far->parameters[it->first];
			if (value <= 0.001) {
				value = 0.1;
				if (pattern)
					value = 1.0;
			} else
				value = value * (1 + delta);

			if (it->first == "base power" && value > min_power)
				value = min_power;

			if (it->first == "base power" && orgvalue > min_power)
				orgvalue = min_power;

			if (value > 5000)
				value = 5000;

//			printf("Trying %s %4.2f -> %4.2f\n", it->first.c_str(), best_so_far->parameters[it->first], value);
			best_so_far->parameters[it->first] = value;

			calculate_params(best_so_far, last_only);
			if (best_so_far->score < best_score || random_disturb(retry)) {
				best_score = best_so_far->score;
				newvalue = value;
				bestparam = it->first;
				changed++;
			}

			value = orgvalue * 1 / (1 + delta);


			if (value > 5000)
				value = 5000;


//			printf("Trying %s %4.2f -> %4.2f\n", it->first.c_str(), orgvalue, value);
			best_so_far->parameters[it->first] = value;

			calculate_params(best_so_far, last_only);
			if (best_so_far->score < best_score || random_disturb(retry)) {
				best_score = best_so_far->score;
				newvalue = value;
				bestparam = it->first;
				changed++;
			}
			best_so_far->parameters[it->first] = orgvalue;

		}
		if (!changed) {
			double mult;
			mult = 0.8;
			if (iterations < 25)
				mult = 0.5;
			delta = delta * mult;
		} else {
			printf("Best parameter is %s \n", bestparam.c_str());
			printf("Changing score from %4.2f to %4.2f\n", best_so_far->score, best_score); 
			printf("Changing value from %4.2f to %4.2f\n", best_so_far->parameters[bestparam], newvalue);
			best_so_far->parameters[bestparam] = newvalue;
		}

		if (delta < 0.001)
			break;
	}

	

	calculate_params(best_so_far, 0);
	printf("Final score %4.2f (%i points)\n", best_so_far->score / past_results.size(), past_results.size());
	if (pattern)
		dump_parameter_bundle(best_so_far);
//	dump_past_results();
}