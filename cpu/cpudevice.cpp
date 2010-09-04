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
	unsigned int i, j;

	return power;	
}

