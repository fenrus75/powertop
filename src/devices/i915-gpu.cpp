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
#include <iostream>
#include <fstream>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include "../lib.h"

using namespace std;

#include "device.h"
#include "i915-gpu.h"
#include "../parameters/parameters.h"
#include "../process/powerconsumer.h"
#include "gpu_rapl_device.h"

#include <string.h>
#include <unistd.h>

i915gpu::i915gpu(): device()
{
	index = get_param_index("gpu-operations");
	rindex = get_result_index("gpu-operations");
}

const char * i915gpu::device_name(void)
{
	if (child_devices.size())
		return "GPU misc";
	else
		return "GPU";
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
	char filename[PATH_MAX];
	class i915gpu *gpu;
	gpu_rapl_device *rapl_dev;

	pt_strcpy(filename, "/sys/kernel/debug/tracing/events/i915/i915_gem_ring_dispatch/format");

	if (access(filename, R_OK) !=0) {
		/* try an older tracepoint */
		pt_strcpy(filename, "/sys/kernel/debug/tracing/events/i915/i915_gem_request_submit/format");
		if (access(filename, R_OK) != 0)
			return;
	}

	register_parameter("gpu-operations");

	gpu = new class i915gpu();
	all_devices.push_back(gpu);

	rapl_dev = new class gpu_rapl_device(gpu);
	if (rapl_dev->device_present())
		all_devices.push_back(rapl_dev);
}



double i915gpu::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;
	double child_power;

	power = 0;
	factor = get_parameter_value(index, bundle);
	util = get_result_value(rindex, result);

	power += util * factor / 100.0;
	for (unsigned int i = 0; i < child_devices.size(); ++i) {
		child_power = child_devices[i]->power_usage(result, bundle);
		if ((power - child_power) > 0.0)
			power -= child_power;
	}

	return power;
}
