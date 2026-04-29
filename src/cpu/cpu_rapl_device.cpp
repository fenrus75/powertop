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
#include "../parameters/parameters.h"
#include "cpu_rapl_device.h"

cpu_rapl_device::cpu_rapl_device(cpudevice *parent, const std::string &classname, const std::string &dev_name, class abstract_cpu *_cpu)
	: cpudevice(classname, dev_name, _cpu),
	  device_valid(false)
{
	if (_cpu)
		rapl = new c_rapl_interface(dev_name, cpu->get_first_cpu());
	else
		rapl = new c_rapl_interface();
	last_time = time(nullptr);
	if (rapl->pp0_domain_present()) {
		device_valid = true;
		parent->add_child(this);
		rapl->get_pp0_energy_status(&last_energy);
	}
}

void cpu_rapl_device::start_measurement(void)
{
	last_time = time(nullptr);

	rapl->get_pp0_energy_status(&last_energy);
}

void cpu_rapl_device::end_measurement(void)
{
	time_t		curr_time = time(nullptr);
	double energy;

	consumed_power = 0.0;
	if ((curr_time - last_time) > 0) {
		rapl->get_pp0_energy_status(&energy);
		consumed_power = (energy-last_energy)/(curr_time-last_time);
		last_energy = energy;
		last_time = curr_time;
	}
}

double cpu_rapl_device::power_usage([[maybe_unused]] struct result_bundle *result, [[maybe_unused]] struct parameter_bundle *bundle)
{
	if (rapl->pp0_domain_present())
		return consumed_power;
	else
		return 0.0;
}

void cpu_rapl_device::collect_json_fields(std::string &_js)
{
    cpudevice::collect_json_fields(_js);
    JSON_KV("last_time", (long)last_time);
    JSON_FIELD(last_energy);
    JSON_FIELD(consumed_power);
    JSON_FIELD(device_valid);
}
