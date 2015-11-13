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
#include <math.h>
#include <unistd.h>
#include <limits.h>

#include "../lib.h"


#include "device.h"
#include "thinkpad-fan.h"
#include "../parameters/parameters.h"
#include "../process/powerconsumer.h"

#include <string.h>
#include <unistd.h>

thinkpad_fan::thinkpad_fan(): device()
{
	start_rate = 0;
	end_rate = 0;
	fan_index = get_param_index("thinkpad-fan");
	fansqr_index = get_param_index("thinkpad-fan-sqr");
	fancub_index = get_param_index("thinkpad-fan-cub");
	r_index = get_result_index("thinkpad-fan");
	register_sysfs_path("/sys/devices/platform/thinkpad_hwmon");
}

void thinkpad_fan::start_measurement(void)
{
	/* read the rpms of the fan */
	start_rate = read_sysfs("/sys/devices/platform/thinkpad_hwmon/fan1_input");
}

void thinkpad_fan::end_measurement(void)
{
	end_rate = read_sysfs("/sys/devices/platform/thinkpad_hwmon/fan1_input");

	report_utilization("thinkpad-fan", utilization());
}


double thinkpad_fan::utilization(void)
{
	return (start_rate+end_rate) / 2;
}

void create_thinkpad_fan(void)
{
	char filename[PATH_MAX];
	class thinkpad_fan *fan;

	pt_strcpy(filename, "/sys/devices/platform/thinkpad_hwmon/fan1_input");

	if (access(filename, R_OK) !=0)
		return;

	register_parameter("thinkpad-fan", 10);
	register_parameter("thinkpad-fan-sqr", 5);
	register_parameter("thinkpad-fan-cub", 10);

	fan = new class thinkpad_fan();
	all_devices.push_back(fan);
}



double thinkpad_fan::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;


	power = 0;
	util = get_result_value(r_index, result);

	if (util < 0)
		util = 0;


	/* physics dictact that fan power goes cubic with the rpms, but there's also a linear component for friction*/
	factor = get_parameter_value(fancub_index, bundle);
	power += factor * pow(util / 3600.0, 3);

	factor = get_parameter_value(fansqr_index, bundle) - 5.0;
	power += factor * pow(util / 3600.0, 2);

	factor = get_parameter_value(fan_index, bundle) - 10.0;
	power += util / 5000.0 * factor;

	if (power <= 0.0)
		power = 0.0;

	return power;
}
