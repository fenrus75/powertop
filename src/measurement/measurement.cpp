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
#include "measurement.h"
#include "acpi.h"
#include "extech.h"
#include "sysfs.h"
#include "../parameters/parameters.h"
#include "../lib.h"

#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <fstream>
#include <unistd.h>

double min_power = 50000.0;

void power_meter::start_measurement(void)
{
}


void power_meter::end_measurement(void)
{
}


double power_meter::joules_consumed(void)
{
	return 0.0;
}

vector<class power_meter *> power_meters;

void start_power_measurement(void)
{
	unsigned int i;
	for (i = 0; i < power_meters.size(); i++)
		power_meters[i]->start_measurement();
}
void end_power_measurement(void)
{
	unsigned int i;
	for (i = 0; i < power_meters.size(); i++)
		power_meters[i]->end_measurement();
}

double global_joules_consumed(void)
{
	bool global_discharging = false;
	double total = 0.0;
	unsigned int i;

	for (i = 0; i < power_meters.size(); i++) {
		global_discharging |= power_meters[i]->is_discharging();
		total += power_meters[i]->joules_consumed();
	}
	/* report global time left if at least one battery is discharging */
	if (!global_discharging)
		return 0.0;

	all_results.power = total;
	if (total < min_power && total > 0.01)
		min_power = total;
	return total;
}

double global_time_left(void)
{
	bool global_discharging = false;
	double total_capacity = 0.0;
	double total_rate = 0.0;
	unsigned int i;
	for (i = 0; i < power_meters.size(); i++) {
		global_discharging |= power_meters[i]->is_discharging();
		total_capacity += power_meters[i]->dev_capacity();
		total_rate += power_meters[i]->joules_consumed();
	}
	/* report global time left if at least one battery is discharging */
	if (!global_discharging)
		return 0.0;

	/* return 0.0 instead of INF+ */
	if (total_rate < 0.001)
		return 0.0;
	return total_capacity / total_rate;
}

void sysfs_power_meters_callback(const char *d_name)
{
	std::string type = read_sysfs_string("/sys/class/power_supply/%s/type", d_name);

	if (type != "Battery" && type != "UPS")
		return;

	class sysfs_power_meter *meter;
	meter = new(std::nothrow) class sysfs_power_meter(d_name);
	if (meter)
		power_meters.push_back(meter);
}

void acpi_power_meters_callback(const char *d_name)
{
	class acpi_power_meter *meter;
	meter = new(std::nothrow) class acpi_power_meter(d_name);
	if (meter)
		power_meters.push_back(meter);
}

void detect_power_meters(void)
{
	process_directory("/sys/class/power_supply", sysfs_power_meters_callback);
	if (power_meters.size() == 0) {
		process_directory("/proc/acpi/battery", acpi_power_meters_callback);
	}
}

void extech_power_meter(const char *devnode)
{
	class extech_power_meter *meter;

	meter = new class extech_power_meter(devnode);

	power_meters.push_back(meter);
}
