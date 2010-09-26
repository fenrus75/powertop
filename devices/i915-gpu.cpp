#include <iostream>
#include <fstream>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>


using namespace std;

#include "device.h"
#include "i915-gpu.h"
#include "../parameters/parameters.h"
#include "../process/powerconsumer.h"

#include <string.h>


i915gpu::i915gpu()
{
	index = get_param_index("gpu-operations");
	rindex = get_result_index("gpu-operations");
}

void i915gpu::start_measurement(void)
{
}

void i915gpu::end_measurement(void)
{
}


double i915gpu::utilization(void)
{
	return  get_result_value(rindex);

}

void create_i915_gpu(void)
{
	char filename[4096];
	class i915gpu *gpu;

	strcpy(filename, "/sys/kernel/debug/tracing/events/i915/i915_gem_request_submit/format");

	if (access(filename, R_OK) !=0)
		return;

	register_parameter("gpu-operations");

	gpu = new class i915gpu();
	all_devices.push_back(gpu);
}



double i915gpu::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double utilization;

	power = 0;
	factor = get_parameter_value(index, bundle);
	utilization = get_result_value(rindex, result);

	power += utilization * factor / 100.0;
	return power;
}