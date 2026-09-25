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
#pragma once


#include <map>
#include <vector>
#include <string>

#include "../devices/device.h"
#include "../lib.h"

#define MAX_KEEP 700
#define MAX_PARAM 750

/*
 * load_results() in persistent.cpp picks an overwrite slot via
 * `50 + (rand() % MAX_KEEP)`, which relies on this exact gap between
 * MAX_PARAM and MAX_KEEP to stay within past_results' bounds (past_results
 * is capped at MAX_PARAM entries). Keep this invariant true, or update
 * that computation if either constant changes.
 */
static_assert(MAX_PARAM - MAX_KEEP == 50, "load_results()'s overflow_index depends on this gap");


struct parameter_bundle
{
	double score = 0.0;
	double guessed_power = 0.0;
	double actual_power = 0.0;

	std::vector<double> parameters;
	std::vector<double> weights;
};

extern struct parameter_bundle all_parameters;
extern std::map <std::string, int> param_index;
extern std::map <std::string, int> result_index;

extern int get_param_index(const std::string &param);
extern int get_result_index(const std::string &param);


extern void register_parameter(const std::string &name, double default_value = 0.00, double weight = 1.0);
extern double get_parameter_value(const std::string &name, const struct parameter_bundle *bundle = &all_parameters);
extern double get_parameter_value(unsigned int index, const struct parameter_bundle *bundle = &all_parameters);
extern void set_parameter_value(const std::string &name, double value, struct parameter_bundle *bundle = &all_parameters);


struct result_bundle
{
	double joules = 0.0;
	double power = 0.0;
	std::vector <double> utilization; /* device name, device utilization %age */
};

extern struct result_bundle all_results;
extern std::vector <struct result_bundle *> past_results;

extern double get_result_value(const std::string &name, const struct result_bundle *bundle = &all_results);
extern double get_result_value(int index, const struct result_bundle *bundle = &all_results);

extern void set_result_value(const std::string &name, double value, struct result_bundle *bundle = &all_results);


extern int result_device_exists(const std::string &name);

extern void report_utilization(const std::string &name, double value, struct result_bundle *bundle = &all_results);
extern void report_utilization(int index, double value, struct result_bundle *bundle = &all_results);


extern void precompute_valid(void);

extern double compute_bundle(struct parameter_bundle *parameters = &all_parameters, struct result_bundle *results = &all_results);


void dump_parameter_bundle(const struct parameter_bundle *parameters = &all_parameters);
void dump_result_bundle(const struct result_bundle *res = &all_results);

extern struct result_bundle * clone_results(const struct result_bundle *bundle);
extern struct parameter_bundle * clone_parameters(const struct parameter_bundle *bundle);

extern void store_results(double duration);
extern void learn_parameters(int iterations, int do_base_power);
extern std::string get_param_directory(const std::string &filename);
extern void save_all_results(const std::string &filename = "saved_results.powertop");
extern void close_results(void);
extern void load_results(const std::string &filename);
extern void save_parameters(const std::string &filename);
extern void load_parameters(const std::string &filename);

extern void dump_past_results(void);
extern double bundle_power(struct parameter_bundle *parameters, struct result_bundle *results);

extern double average_power(void);

extern int utilization_power_valid(const std::string &u);
extern int utilization_power_valid(int index);
extern double calculate_params(struct parameter_bundle *params = &all_parameters);
int global_power_valid(void);


extern int global_power_override;

