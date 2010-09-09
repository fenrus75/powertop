#include "parameters.h"
#include "../measurement/measurement.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern int debug_learning;

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
	
	if ( (rand() % 500) == 7)
		return 1;
	return 0;
}

static unsigned int previous_measurements;

/* leaks like a sieve */
void learn_parameters(int iterations, const char *pattern)
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

	if (1.0 * best_score / past_results.size() < 4 && delta > 0.05)
		delta = 0.05;

	if (debug_learning)
		printf("Delta starts at %5.3f\n", delta);

	if (best_so_far->parameters["base power"] > min_power)
		best_so_far->parameters["base power"] = min_power;

	/* We want to give up a little of base power, to give other parameters room to change;
	   base power is the end post for everything after all 
         */
	best_so_far->parameters["base power"] = best_so_far->parameters["base power"] * 0.95;

	while (retry--) {
		int changed  = 0;
		string bestparam;
		double newvalue;

		bestparam = "";

		calculate_params(best_so_far);
		best_score = best_so_far->score;

		
	        for (it = best_so_far->parameters.begin(); it != best_so_far->parameters.end(); it++) {
			double value, orgvalue;

			if (pattern) {
				if (strstr(it->first.c_str(), pattern) == NULL && it->first != "base power")
					continue;
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

			calculate_params(best_so_far);
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

			calculate_params(best_so_far);
			if (best_so_far->score + 0.00001 < best_score || random_disturb(retry)) {
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
			if (debug_learning) {
				printf("Retry is %i \n", retry);
					printf("delta is %5.4f\n", delta);
				printf("Best parameter is %s \n", bestparam.c_str());
				printf("Changing score from %4.3f to %4.3f\n", best_so_far->score, best_score); 
				printf("Changing value from %4.3f to %4.3f\n", best_so_far->parameters[bestparam], newvalue);
			}
			best_so_far->parameters[bestparam] = newvalue;
		}

		if (delta < 0.001)
			break;
	}


	/* now we weed out all parameters that don't have value */

	best_score = best_so_far->score;

		
	for (it = best_so_far->parameters.begin(); it != best_so_far->parameters.end(); it++) {
		double orgvalue;

		orgvalue = best_so_far->parameters[it->first];


		best_so_far->parameters[it->first] = 0.0;

		calculate_params(best_so_far);
		if (best_so_far->score > best_score) {
				best_so_far->parameters[it->first] = orgvalue;
		} else {
			best_score = best_so_far->score;
		}

	}
	calculate_params(best_so_far);
	if (debug_learning)
		printf("Final score %4.2f (%i points)\n", best_so_far->score / past_results.size(), past_results.size());
//	dump_parameter_bundle(best_so_far);
//	dump_past_results();
}