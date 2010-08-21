#ifndef __INCLUDE_GUARD_PARAMETERS_H_
#define __INCLUDE_GUARD_PARAMETERS_H_



void register_parameter(char *name, double default_value = 0.0);
double get_parameter_value(char *name);

#endif