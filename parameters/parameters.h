#ifndef __INCLUDE_GUARD_PARAMETERS_H_
#define __INCLUDE_GUARD_PARAMETERS_H_


#include <map>
#include <vector>
#include <string>

#include "string.h"
#include "../devices/device.h"
#include "../lib.h"

using namespace std;



struct parameter_bundle
{
	double score;
	double guessed_power;
	double actual_power;

	vector<double> parameters; 
};

extern struct parameter_bundle all_parameters;
extern map <string, int> param_index;
extern map <string, int> result_index;

extern int get_param_index(const char *param);
extern int get_result_index(const char *param);


extern void register_parameter(const char *name, double default_value = 0.00);
extern double get_parameter_value(const char *name, struct parameter_bundle *bundle = &all_parameters);
extern double get_parameter_value(int index, struct parameter_bundle *bundle = &all_parameters);
extern void set_parameter_value(const char *name, double value, struct parameter_bundle *bundle = &all_parameters);


struct result_bundle
{
	double power;
	vector <double> utilization; /* device name, device utilization %age */
};

extern struct result_bundle all_results;
extern vector <struct result_bundle *> past_results;

extern double get_result_value(const char *name, struct result_bundle *bundle = &all_results);
extern double get_result_value(int index, struct result_bundle *bundle = &all_results);

extern void set_result_value(const char *name, double value, struct result_bundle *bundle = &all_results);


extern int result_device_exists(const char *name);

extern void report_utilization(const char *name, double value, struct result_bundle *bundle = &all_results);


extern double compute_bundle(struct parameter_bundle *parameters = &all_parameters, struct result_bundle *results = &all_results);


void dump_parameter_bundle(struct parameter_bundle *patameters = &all_parameters);
void dump_result_bundle(struct result_bundle *res = &all_results);

extern struct result_bundle * clone_results(struct result_bundle *bundle);
extern struct parameter_bundle * clone_parameters(struct parameter_bundle *bundle);

extern void store_results(void);
extern void learn_parameters(int iterations = 100);
extern void save_all_results(const char *filename);
extern void load_results(const char *filename);
extern void save_parameters(const char *filename);
extern void load_parameters(const char *filename);

extern void dump_past_results(void);
extern double bundle_power(struct parameter_bundle *parameters, struct result_bundle *results);

extern double average_power(void);

extern int utilization_power_valid(const char *u);
extern int utilization_power_valid(int index);

#endif
