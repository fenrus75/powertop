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
#include "../parameters/parameters.h"
#include "../lib.h"

#include <sys/types.h>
#include <dirent.h>

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

double power_meter::time_left(void)
{
	double cap, rate;
	double left;

	cap = dev_capacity();
	rate = joules_consumed();

	if (cap < 0.001)
		return 0.0;

	left = cap / rate;

	return left;
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
	double total = 0.0;
	unsigned int i;
	for (i = 0; i < power_meters.size(); i++)
		total += power_meters[i]->joules_consumed();

	all_results.power = total;	
	if (total < min_power && total > 0.01)
		min_power = total;
	return total;
}

double global_time_left(void)
{
	double total = 0.0;
	unsigned int i;
	for (i = 0; i < power_meters.size(); i++)
		total += power_meters[i]->time_left();

	return total;
}

void power_meters_callback(const char *d_name)
{
	class acpi_power_meter *meter;
	meter = new(std::nothrow) class acpi_power_meter(d_name);
	if (meter)
		power_meters.push_back(meter);
}

void detect_power_meters(void)
{
	process_directory("/proc/acpi/battery", power_meters_callback);
}

void extech_power_meter(const char *devnode)
{
	DIR *dir;
	struct dirent *entry;

	class extech_power_meter *meter;

	meter = new class extech_power_meter(devnode);

	power_meters.push_back(meter);
}
