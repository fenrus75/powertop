/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */
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

static int try_zero(double value)
{
	if (value > 0.01)
	if ( (rand() % 100) == 1)
		return 1;

	if ( (rand() % 5) == 1)
		return 1;
	return 0;
}

static unsigned int previous_measurements;

static void weed_empties(struct parameter_bundle *best_so_far)
{
	double best_score;
	unsigned int i;

	best_score = best_so_far->score;


	for (i = 0; i < best_so_far->parameters.size(); i++) {
		double orgvalue;

		orgvalue = best_so_far->parameters[i];


		best_so_far->parameters[i] = 0.0;

		calculate_params(best_so_far);
		if (best_so_far->score > best_score) {
				best_so_far->parameters[i] = orgvalue;
		} else {
			best_score = best_so_far->score;
		}

	}
	calculate_params(best_so_far);

}

/* leaks like a sieve */
void learn_parameters(int iterations, int do_base_power)
{
	struct parameter_bundle *best_so_far;
	double best_score = 10000000000000000.0;
	int retry = iterations;
	int prevparam = -1;
	int locked = 0;
	static unsigned int bpi = 0;
	unsigned int i;
	time_t start;

	if (global_fixed_parameters)
		return;


	/* don't start fitting anything until we have at least 1 more measurement than we have parameters */
	if (past_results.size() <= all_parameters.parameters.size())
		return;



//	if (past_results.size() == previous_measurements)
//		return;

	precompute_valid();


	previous_measurements = past_results.size();

	double delta = 0.50;

	best_so_far = &all_parameters;

	if (!bpi)
		bpi = get_param_index("base power");

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

	if (best_so_far->parameters[bpi] > min_power * 0.9)
		best_so_far->parameters[bpi] = min_power * 0.9;

	/* We want to give up a little of base power, to give other parameters room to change;
	   base power is the end post for everything after all
         */
	if (do_base_power && !debug_learning)
		best_so_far->parameters[bpi] = best_so_far->parameters[bpi] * 0.9998;

	start = time(NULL);

	while (retry--) {
		int changed  = 0;
		int bestparam;
		double newvalue = 0;
		double orgscore;
		double weight;

		bestparam = -1;

		if (time(NULL) - start > 1 && !debug_learning)
			retry = 0;

		calculate_params(best_so_far);
		orgscore = best_score = best_so_far->score;


	        for (i = 1; i < best_so_far->parameters.size(); i++) {
			double value, orgvalue;

			weight = delta * best_so_far->weights[i];

			orgvalue = value = best_so_far->parameters[i];
			if (value <= 0.001) {
				value = 0.1;
			} else
				value = value * (1 + weight);

			if (i == bpi && value > min_power)
				value = min_power;

			if (i == bpi && orgvalue > min_power)
				orgvalue = min_power;

			if (value > 5000)
				value = 5000;

//			printf("Trying %s %4.2f -> %4.2f\n", param.c_str(), best_so_far->parameters[param], value);
			best_so_far->parameters[i] = value;

			calculate_params(best_so_far);
			if (best_so_far->score < best_score || random_disturb(retry)) {
				best_score = best_so_far->score;
				newvalue = value;
				bestparam = i;
				changed++;
			}

			value = orgvalue * 1 / (1 + weight);

			if (value < 0.0001)
				value = 0.0;

			if (try_zero(value))
				value = 0.0;


			if (value > 5000)
				value = 5000;


//			printf("Trying %s %4.2f -> %4.2f\n", param.c_str(), orgvalue, value);

			if (orgvalue != value) {
				best_so_far->parameters[i] = value;

				calculate_params(best_so_far);

				if (best_so_far->score + 0.00001 < best_score || (random_disturb(retry) && value > 0.0)) {
					best_score = best_so_far->score;
					newvalue = value;
					bestparam = i;
					changed++;
				}
			}
			best_so_far->parameters[i] = orgvalue;

		}
		if (!changed) {
			double mult;

			if (!locked) {
				mult = 0.8;
				if (iterations < 25)
					mult = 0.5;
				delta = delta * mult;
			}
			locked = 0;
			prevparam = -1;
		} else {
			if (debug_learning) {
				printf("Retry is %i \n", retry);
					printf("delta is %5.4f\n", delta);
				printf("Best parameter is %i \n", bestparam);
				printf("Changing score from %4.3f to %4.3f\n", orgscore, best_score);
				printf("Changing value from %4.3f to %4.3f\n", best_so_far->parameters[bestparam], newvalue);
			}
			best_so_far->parameters[bestparam] = newvalue;
			if (prevparam == bestparam)
				delta = delta * 1.1;
			prevparam = bestparam;
			locked = 1;
		}

		if (delta < 0.001 && !locked)
			break;

		if (retry % 50 == 49)
			weed_empties(best_so_far);
	}


	/* now we weed out all parameters that don't have value */
	if (iterations > 50)
		weed_empties(best_so_far);

	if (debug_learning)
		printf("Final score %4.2f (%i points)\n", best_so_far->score / past_results.size(), (int)past_results.size());
//	dump_parameter_bundle(best_so_far);
//	dump_past_results();
}