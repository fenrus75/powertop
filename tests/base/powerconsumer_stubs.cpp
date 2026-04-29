/*
 * Stubs for power_consumer base class test.
 * Only the symbols that powerconsumer.cpp actually references are provided.
 */
#include "parameters/parameters.h"

struct parameter_bundle all_parameters;
struct result_bundle all_results;

double get_parameter_value([[maybe_unused]] const std::string &name,
			   [[maybe_unused]] struct parameter_bundle *bundle)
{
	return 0.0;
}

double get_parameter_value([[maybe_unused]] unsigned int index,
			   [[maybe_unused]] struct parameter_bundle *bundle)
{
	return 0.0;
}

std::vector<class power_consumer *> all_power;
