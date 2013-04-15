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
 *	Srinivas Pandruvada<Srinivas.Pandruvada@linux.intel.com>
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../parameters/parameters.h"
#include "gpu_rapl_device.h"

gpu_rapl_device::gpu_rapl_device(i915gpu *parent)
	: i915gpu(),
	  device_valid(false)
{
	last_time = time(NULL);
	if (rapl.pp1_domain_present()) {
		device_valid = true;
		parent->add_child(this);
		rapl.get_pp1_energy_status(&last_energy);
	}
}

void gpu_rapl_device::start_measurement(void)
{
	last_time = time(NULL);

	rapl.get_pp1_energy_status(&last_energy);
}

void gpu_rapl_device::end_measurement(void)
{
	time_t		curr_time = time(NULL);
	double energy;

	consumed_power = 0.0;
	if ((curr_time - last_time) > 0) {
		rapl.get_pp1_energy_status(&energy);
		consumed_power = (energy-last_energy)/(curr_time-last_time);
		last_energy = energy;
		last_time = curr_time;
	}
}

double gpu_rapl_device::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	if (rapl.pp1_domain_present())
		return consumed_power;
	else
		return 0.0;
}
