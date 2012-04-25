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


using namespace std;

#include "device.h"
#include "backlight.h"
#include "../parameters/parameters.h"

#include <string.h>


backlight::backlight(char *_name, char *path): device()
{
	char devname[128];
	min_level = 0;
	max_level = 0;
	start_level = 0;
	end_level = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));
	register_sysfs_path(sysfs_path);
	sprintf(devname, "backlight:%s", _name);
	strncpy(name, devname, sizeof(name));
	r_index = get_result_index(name);
	r_index_power = 0;
}

void backlight::start_measurement(void)
{
	char filename[4096];
	ifstream file;

	sprintf(filename, "%s/max_brightness", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> max_level;
	}
	file.close();

	sprintf(filename, "%s/actual_brightness", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> start_level;
		file.close();
	}
}

static int dpms_screen_on(void)
{
	DIR *dir;
	struct dirent *entry;
	char filename[4096];
	char line[4096];
	ifstream file;

	dir = opendir("/sys/class/drm/card0");
	if (!dir)
		return 1;
	while (1) {
		entry = readdir(dir);
		if (!entry)
			break;

		sprintf(filename, "/sys/class/drm/card0/%s/enabled", entry->d_name);
		file.open(filename, ios::in);
		if (!file)
			continue;
		file.getline(line, 4096);
		file.close();
		if (strcmp(line, "enabled") != 0)
			continue;
		sprintf(filename, "/sys/class/drm/card0/%s/dpms", entry->d_name);
		file.open(filename, ios::in);
		if (!file)
			continue;
		file.getline(line, 4096);
		file.close();
		if (strcmp(line, "On") == 0) {
			closedir(dir);
			return 1;
		}
	}
	closedir(dir);
	return 0;
}

void backlight::end_measurement(void)
{
	char filename[4096];
	char powername[4096];
	ifstream file;
	double p;
	int _backlight = 0;

	sprintf(filename, "%s/actual_brightness", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> end_level;
	}
	file.close();

	if (dpms_screen_on()) {
		p = 100.0 * (end_level + start_level) / 2 / max_level;
		_backlight = 100;
	} else {
		p = 0;
	}

	report_utilization(name, p);
	sprintf(powername, "%s-power", name);
	report_utilization(powername, _backlight);
}


double backlight::utilization(void)
{
	double p;

	p = 100.0 * (end_level + start_level) / 2 / max_level;
	return p;
}

const char * backlight::device_name(void)
{
	return name;
}

void create_all_backlights(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];

	dir = opendir("/sys/class/backlight/");
	if (!dir)
		return;
	while (1) {
		class backlight *bl;
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;
		sprintf(filename, "/sys/class/backlight/%s", entry->d_name);
		bl = new class backlight(entry->d_name, filename);
		all_devices.push_back(bl);
		register_parameter("backlight");
		register_parameter("backlight-power");
		register_parameter("backlight-boost-40", 0, 0.5);
		register_parameter("backlight-boost-80", 0, 0.5);
		register_parameter("backlight-boost-100", 0, 0.5);
	}
	closedir(dir);

}



double backlight::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double _utilization;
	char powername[4096];
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
		sprintf(powername, "%s-power", name);
		r_index_power = get_result_index(powername);
	}
	_utilization = get_result_value(r_index_power, result);

	power += _utilization * factor / 100.0;

	return power;
}
