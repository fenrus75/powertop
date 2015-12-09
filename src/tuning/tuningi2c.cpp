/*
 * Copyright 2015, Intel Corporation
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
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 *	Daniel Leung <daniel.leung@linux.intel.com>
 */

#include "tuning.h"
#include "tunable.h"
#include "unistd.h"
#include "tuningi2c.h"
#include <string.h>
#include <dirent.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <limits.h>

#include "../lib.h"
#include "../devices/runtime_pm.h"

i2c_tunable::i2c_tunable(const char *path, const char *name, bool is_adapter) : tunable("", 0.9, _("Good"), _("Bad"), _("Unknown"))
{
	ifstream file;
	char filename[PATH_MAX];
	string devname;

	snprintf(filename, PATH_MAX, "%s/name", path);
	file.open(filename, ios::in);
	if (file) {
		getline(file, devname);
		file.close();
	}

	if (is_adapter) {
		snprintf(i2c_path, PATH_MAX, "%s/device/power/control", path);
		snprintf(filename, PATH_MAX, "%s/device", path);
	} else {
		snprintf(i2c_path, PATH_MAX,  "%s/power/control", path);
		snprintf(filename, PATH_MAX, "%s/device", path);
	}

	if (device_has_runtime_pm(filename))
		snprintf(desc, 4096, _("Runtime PM for I2C %s %s (%s)"), (is_adapter ? _("Adapter") : _("Device")), name, (devname.empty() ? "" : devname.c_str()));
	else
		snprintf(desc, 4096, _("I2C %s %s has no runtime power management"), (is_adapter ? _("Adapter") : _("Device")), name);

	snprintf(toggle_good, 4096, "echo 'auto' > '%s';", i2c_path);
	snprintf(toggle_bad, 4096, "echo 'on' > '%s';", i2c_path);
}

int i2c_tunable::good_bad(void)
{
	string content;

	content = read_sysfs_string(i2c_path);

	if (strcmp(content.c_str(), "auto") == 0)
		return TUNE_GOOD;

	return TUNE_BAD;
}

void i2c_tunable::toggle(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		write_sysfs(i2c_path, "on");
		return;
	}

	write_sysfs(i2c_path, "auto");
}

const char *i2c_tunable::toggle_script(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		return toggle_bad;
	}

	return toggle_good;
}

static void add_i2c_callback(const char *d_name)
{
	class i2c_tunable *i2c;
	char filename[PATH_MAX];
	bool is_adapter = false;

	snprintf(filename, PATH_MAX, "/sys/bus/i2c/devices/%s/new_device", d_name);
	if (access(filename, W_OK) == 0)
		is_adapter = true;

	snprintf(filename, PATH_MAX, "/sys/bus/i2c/devices/%s", d_name);
	i2c = new class i2c_tunable(filename, d_name, is_adapter);

	if (is_adapter)
		snprintf(filename, PATH_MAX, "/sys/bus/i2c/devices/%s/device", d_name);
	else
		snprintf(filename, PATH_MAX, "/sys/bus/i2c/devices/%s", d_name);

	if (device_has_runtime_pm(filename))
		all_tunables.push_back(i2c);
	else
		all_untunables.push_back(i2c);
}

void add_i2c_tunables(void)
{
	process_directory("/sys/bus/i2c/devices/", add_i2c_callback);
}
