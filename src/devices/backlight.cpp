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
#include <limits.h>


#include "device.h"
#include "backlight.h"
#include "../parameters/parameters.h"

#include <format>


backlight::backlight(const std::string &_name, const std::string &path): device()
{
	min_level = 0;
	max_level = 0;
	start_level = 0;
	end_level = 0;
	sysfs_path = path;
	register_sysfs_path(sysfs_path);
	name = std::format("backlight:{}", _name);
	r_index = get_result_index(name);
	r_index_power = 0;
}

void backlight::start_measurement(void)
{
	max_level = read_sysfs(std::format("{}/max_brightness", sysfs_path));
	start_level = read_sysfs(std::format("{}/actual_brightness", sysfs_path));
}

static int dpms_screen_on(void)
{
	DIR *dir;
	struct dirent *entry;
	std::string line;

	dir = opendir("/sys/class/drm/card0");
	if (!dir)
		return 1;
	while (1) {
		entry = readdir(dir);
		if (!entry)
			break;

		if (!std::string_view(entry->d_name).starts_with("card"))
			continue;

		line = read_sysfs_string(std::format("/sys/class/drm/card0/{}/enabled", entry->d_name));
		if (line != "enabled")
			continue;

		line = read_sysfs_string(std::format("/sys/class/drm/card0/{}/dpms", entry->d_name));
		if (line == "On") {
			closedir(dir);
			return 1;
		}
	}
	closedir(dir);
	return 0;
}

void backlight::end_measurement(void)
{
	double p;
	int _backlight = 0;

	end_level = read_sysfs(std::format("{}/actual_brightness", sysfs_path));

	if (dpms_screen_on()) {
		if (max_level > 0)
			p = 100.0 * (end_level + start_level) / 2 / max_level;
		else
			p = 0;
		_backlight = 100;
	} else {
		p = 0;
	}

	report_utilization(name, p);
	report_utilization(std::format("{}-power", name), _backlight);
}


double backlight::utilization(void)
{
	if (max_level <= 0)
		return 0.0;
	return 100.0 * (end_level + start_level) / 2 / max_level;
}

static void create_all_backlights_callback(const std::string &d_name)
{
	class backlight *bl;
	bl = new backlight(d_name, std::format("/sys/class/backlight/{}", d_name));
	all_devices.push_back(bl);
}

void create_all_backlights(void)
{
	process_directory("/sys/class/backlight/", create_all_backlights_callback);
	register_parameter("backlight");
	register_parameter("backlight-power");
	register_parameter("backlight-boost-40", 0, 0.5);
	register_parameter("backlight-boost-80", 0, 0.5);
	register_parameter("backlight-boost-100", 0, 0.5);
}

double backlight::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double _utilization;
	static int bl_index = 0, blp_index = 0, bl_boost_index40 = 0, bl_boost_index80, bl_boost_index100;

	if (!bl_index)
		bl_index = get_param_index("backlight");
	if (!blp_index)
		blp_index = get_param_index("backlight-power");
	if (!bl_boost_index40)
		bl_boost_index40 = get_param_index("backlight-boost-40");
	if (!bl_boost_index80)
		bl_boost_index80 = get_param_index("backlight-boost-80");
	if (!bl_boost_index100)
		bl_boost_index100 = get_param_index("backlight-boost-100");

	power = 0;
	factor = get_parameter_value(bl_index, bundle);
	_utilization = get_result_value(r_index, result);

	power += _utilization * factor / 100.0;

	/*
	 * most machines have a non-linear backlight scale. to compensate, add a fixed value
	 * once the brightness hits 40% and 80%
	 */

	if (_utilization >=99)
		power += get_parameter_value(bl_boost_index100, bundle);
	else if (_utilization >=80)
		power += get_parameter_value(bl_boost_index80, bundle);
	else if (_utilization >=40)
		power += get_parameter_value(bl_boost_index40, bundle);

	factor = get_parameter_value(blp_index, bundle);

	if (!r_index_power) {
		r_index_power = get_result_index(std::format("{}-power", name));
	}
	_utilization = get_result_value(r_index_power, result);

	power += _utilization * factor / 100.0;

	return power;
}
