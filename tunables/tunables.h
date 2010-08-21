#ifndef __INCLUDE_GUARD_TUNABLES_H_
#define __INCLUDE_GUARD_TUNABLES_H_



void register_tunable(char *name, double default_value = 0.0);

double get_tunable_value(char *name);

#endif