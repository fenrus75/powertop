#include "cpudevice.h"
#include <string.h>


cpudevice::cpudevice(const char *classname, const char *device_name)
{
	strcpy(_class, classname);
	strcpy(_cpuname, device_name);
}


double cpudevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power = 0;

	double factor;

	return 0;	
}

