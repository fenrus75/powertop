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
}


double cpudevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	unsigned int i, j;

	double power;
	double factor;
	double utilization;

	power = 0;
	factor = get_parameter_value("cpu-wakeups", bundle);
	utilization = get_result_value("cpu-wakeups", result);

        power += utilization * factor / 1000.0;

        factor = get_parameter_value("cpu-consumption", bundle);
        utilization = get_result_value("cpu-consumption", result);

        power += utilization * factor / 100.0;


	return power;	
}

