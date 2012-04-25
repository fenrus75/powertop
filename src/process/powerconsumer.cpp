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

#include "powerconsumer.h"
#include "process.h"
#include "../parameters/parameters.h"

double power_consumer::Witts(void)
{
	double cost;
	double timecost;
	double wakeupcost;
	double gpucost;
	double disk_cost;
	double hard_disk_cost;
	double xwake_cost;

	if (child_runtime > accumulated_runtime)
		child_runtime = 0;

	timecost = get_parameter_value("cpu-consumption");
	wakeupcost = get_parameter_value("cpu-wakeups");
	gpucost = get_parameter_value("gpu-operations");
	disk_cost = get_parameter_value("disk-operations");
	hard_disk_cost = get_parameter_value("disk-operations-hard");
	xwake_cost = get_parameter_value("xwakes");

	cost = 0;

	cost += wakeupcost * wake_ups / 10000.0;
	cost += ( (accumulated_runtime - child_runtime) / 1000000000.0) * timecost;
	cost += gpucost * gpu_ops / 100.0;
	cost += hard_disk_cost * hard_disk_hits / 100.0;
	cost += disk_cost * disk_hits / 100.0;
	cost += xwake_cost * xwakes / 100.0;

	cost = cost / measurement_time;

	cost += power_charge;

	return cost;
}

power_consumer::power_consumer(void)
{
	accumulated_runtime = 0;
	child_runtime = 0;
	disk_hits = 0;
	wake_ups = 0;
	gpu_ops = 0;
	hard_disk_hits = 0;
	xwakes = 0;
	waker = NULL;
	last_waker = NULL;
	power_charge = 0.0;
}

double power_consumer::usage(void)
{
	double t;
	t = (accumulated_runtime - child_runtime) / 1000000.0 / measurement_time;
	if (t < 0.7)
		t = t * 1000;
	return t;
}

const char * power_consumer::usage_units(void)
{
	double t;
	t = (accumulated_runtime - child_runtime) / 1000000.0 / measurement_time;
	if (t < 0.7) {
		if (utf_ok)
			return " µs/s";
		else
			return " us/s";
	}
	return " ms/s";
}
