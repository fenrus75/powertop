#include "cpudevice.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../parameters/parameters.h"


cpudevice::cpudevice(const char *classname, const char *device_name, class abstract_cpu *_cpu)
{
	strcpy(_class, classname);
	strcpy(_cpuname, device_name);
	cpu = _cpu;
	wake_index = get_param_index("cpu-wakeups");;
	consumption_index = get_param_index("cpu-consumption");;
	r_wake_index = get_result_index("cpu-wakeups");;
	r_consumption_index = get_result_index("cpu-consumption");;
}


double cpudevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	unsigned int i, j;

	double power;
	double factor;
	double utilization;

	power = 0;
	factor = get_parameter_value(wake_index, bundle);
	utilization = get_result_value(r_wake_index, result);

        power += utilization * factor / 10000.0;

        factor = get_parameter_value(consumption_index, bundle);
        utilization = get_result_value(r_consumption_index, result);

        power += utilization * factor;


	return power;	
}

double	cpudevice::utilization(void)
{
	double utilization;
	utilization = get_result_value(r_consumption_index);

	return utilization * 100;

}
