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
#include <format>
#include "../lib.h"
#include "../parameters/parameters.h"


cpudevice::cpudevice(const std::string &classname, const std::string &dev_name, class abstract_cpu *_cpu)
{
	_class = classname;
	_cpuname = dev_name;
	cpu = _cpu;
	wake_index = get_param_index("cpu-wakeups");
	consumption_index = get_param_index("cpu-consumption");
	r_wake_index = get_result_index("cpu-wakeups");
	r_consumption_index = get_result_index("cpu-consumption");
}

std::string cpudevice::device_name(void)
{
	return _cpuname;
}

std::string cpudevice::human_name(void)
{
	if (!guilty.empty())
		return std::format("{} ({})", _cpuname, guilty);

	return _cpuname;
}

double cpudevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double _utilization;
	double child_power;

	power = 0;
	factor = get_parameter_value(wake_index, bundle);
	_utilization = get_result_value(r_wake_index, result);

	power += _utilization * factor / 10000.0;

	factor = get_parameter_value(consumption_index, bundle);
	_utilization = get_result_value(r_consumption_index, result);

	power += _utilization * factor;

	for (unsigned int i = 0; i < child_devices.size(); ++i) {
		child_power = child_devices[i]->power_usage(result, bundle);
		if ((power - child_power) > 0.0)
			power -= child_power;
	}

	return power;
}

double	cpudevice::utilization(void)
{
	double _utilization;
	_utilization = get_result_value(r_consumption_index);

	return _utilization * 100;

}

void cpudevice::collect_json_fields(std::string &_js)
{
    device::collect_json_fields(_js);
    JSON_FIELD(_class);
    JSON_FIELD(_cpuname);
    std::string ps = "[";
    for (size_t i = 0; i < params.size(); i++) {
        ps += "\"" + pt_json_escape(params[i]) + "\"";
        if (i + 1 < params.size()) ps += ",";
    }
    ps += "]";
    _js += "\"params\":" + ps + ",";
    JSON_FIELD(wake_index);
    JSON_FIELD(consumption_index);
    JSON_FIELD(r_wake_index);
    JSON_FIELD(r_consumption_index);
    JSON_ARRAY("child_devices", child_devices);
}
