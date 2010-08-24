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
	double power = 0;
	unsigned int i;

        for (i = 0; i < cpu->pstates.size(); i ++) {
		double factor;
		double util;
		char buffer[128];
                sprintf(buffer,"package-freq-%s", cpu->pstates[i]->human_name);
		factor = get_parameter_value(buffer, bundle);
                sprintf(buffer,"package-%i-freq-%s", cpu->number, cpu->pstates[i]->human_name);
		util = get_result_value(buffer, result);

		power += factor * util / 100.0;
        }
	return power;	
}

