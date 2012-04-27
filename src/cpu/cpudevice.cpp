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
#include "cpudevice.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../parameters/parameters.h"


cpudevice::cpudevice(const char *classname, const char *dev_name, class abstract_cpu *_cpu)
{
	strcpy(_class, classname);
	strcpy(_cpuname, dev_name);
	cpu = _cpu;
	wake_index = get_param_index("cpu-wakeups");;
	consumption_index = get_param_index("cpu-consumption");;
	r_wake_index = get_result_index("cpu-wakeups");;
	r_consumption_index = get_result_index("cpu-consumption");;
}


double cpudevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double _utilization;

	power = 0;
	factor = get_parameter_value(wake_index, bundle);
	_utilization = get_result_value(r_wake_index, result);

	power += _utilization * factor / 10000.0;

	factor = get_parameter_value(consumption_index, bundle);
	_utilization = get_result_value(r_consumption_index, result);

	power += _utilization * factor;

	return power;
}

double	cpudevice::utilization(void)
{
	double _utilization;
	_utilization = get_result_value(r_consumption_index);

	return _utilization * 100;

}
