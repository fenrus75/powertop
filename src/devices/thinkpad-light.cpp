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
#include "thinkpad-light.h"
#include "../parameters/parameters.h"
#include "../process/powerconsumer.h"

#include <string.h>
#include <unistd.h>

thinkpad_light::thinkpad_light(): device()
{
	start_rate = 0;
	end_rate = 0;
	light_index = get_param_index("thinkpad-light");
	r_index = get_result_index("thinkpad-light");
	register_sysfs_path("/sys/devices/platform/thinkpad_acpi/leds/tpacpi::thinklight");
}

void thinkpad_light::start_measurement(void)
{
	/* read the rpms of the light */
	start_rate = read_sysfs("/sys/devices/platform/thinkpad_acpi/leds/tpacpi::thinklight/brightness");
}

void thinkpad_light::end_measurement(void)
{
	end_rate   = read_sysfs("/sys/devices/platform/thinkpad_acpi/leds/tpacpi::thinklight/brightness");

	report_utilization("thinkpad-light", utilization());
}


double thinkpad_light::utilization(void)
{
	return (start_rate+end_rate) / 2.55 / 2.0;
}

void create_thinkpad_light(void)
{
	char filename[PATH_MAX];
	class thinkpad_light *light;

	strcpy(filename, "/sys/devices/platform/thinkpad_acpi/leds/tpacpi::thinklight/brightness");

	if (access(filename, R_OK) !=0)
		return;

	register_parameter("thinkpad-light", 10);

	light = new class thinkpad_light();
	all_devices.push_back(light);
}



double thinkpad_light::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;


	power = 0;
	util = get_result_value(r_index, result);

	if (util < 0)
		util = 0;


	factor = get_parameter_value(light_index, bundle) - 10.0;
	power += util / 100.0 * factor;

	if (power <= 0.0)
		power = 0.0;

	return power;
}
