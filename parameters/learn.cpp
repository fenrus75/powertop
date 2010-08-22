#include "parameters.h"

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
	double best_score = 10000000000000000;
        map<const char *, double>::iterator it;
	int retry = 50;

	double delta = 0.20;

	best_so_far = clone_parameters(&all_parameters);

	calculate_params(best_so_far);
	printf("Starting score %5.1f\n", best_so_far->score);
	best_score = best_so_far->score;
	dump_parameter_bundle(best_so_far);


	while (retry--) {
	        for (it = best_so_far->parameters.begin(); it != best_so_far->parameters.end(); it++) {
			struct parameter_bundle *clone;
			double value;

			clone = clone_parameters(best_so_far);
			value = clone->parameters[it->first];
			if (value == 0.0)
				value = 0.1;
			else
				value = value * (1 + delta);

			printf("Trying %s %5.1f -> %5.1f\n", it->first, clone->parameters[it->first], value);
			clone->parameters[it->first] = value;

			calculate_params(clone);
			if (clone->score < best_score) {
				best_score = clone->score;
				printf("Better score %5.1f\n", best_so_far->score);
				dump_parameter_bundle(best_so_far);
				best_so_far = clone;
				break;
			}
		}
	        for (it = best_so_far->parameters.begin(); it != best_so_far->parameters.end(); it++) {
			struct parameter_bundle *clone;
			double value;

			clone = clone_parameters(best_so_far);
			value = clone->parameters[it->first];
			if (value == 0.0)
				value = 0.0;
			else
				value = value * 1 / (1 + delta);

			printf("Trying %s %5.1f -> %5.1f\n", it->first, clone->parameters[it->first], value);
			clone->parameters[it->first] = value;

			calculate_params(clone);
			if (clone->score < best_score) {
				best_score = clone->score;
				printf("Better score %5.1f\n", best_so_far->score);
				dump_parameter_bundle(best_so_far);
				best_so_far = clone;
				break;
			}
		}
		delta = delta / 2;
       	 }

	

	calculate_params(best_so_far);
	printf("Final score %5.1f\n", best_so_far->score);
	dump_parameter_bundle(best_so_far);
}