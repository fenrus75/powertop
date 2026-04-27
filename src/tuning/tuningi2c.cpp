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
#include <format>

#include "../lib.h"
#include "../devices/runtime_pm.h"
i2c_tunable::i2c_tunable(const string &path, const string &name, bool is_adapter) : tunable("", 0.9, _("Good"), _("Bad"), _("Unknown"))
{
	std::string filename;
	std::string devname;

	devname = read_sysfs_string(std::format("{}/name", path));

	if (is_adapter) {
		i2c_path = std::format("{}/device/power/control", path);
		filename = std::format("{}/device", path);
	} else {
		i2c_path = std::format("{}/power/control", path);
		filename = std::format("{}/device", path);
	}

	if (device_has_runtime_pm(filename)) {
		desc = pt_format(_("Runtime PM for I2C {} {} ({})"), (is_adapter ? _("Adapter") : _("Device")), name, devname);
	} else {
		desc = pt_format(_("I2C {} {} has no runtime power management"), (is_adapter ? _("Adapter") : _("Device")), name);
	}

	toggle_good = std::format("echo 'auto' > '{}';", i2c_path);
	toggle_bad = std::format("echo 'on' > '{}';", i2c_path);
}

int i2c_tunable::good_bad(void)
{
	std::string content;

	content = read_sysfs_string(i2c_path);

	if (content == "auto")
		return TUNE_GOOD;

	return TUNE_BAD;
}

void i2c_tunable::toggle(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		write_sysfs(i2c_path.c_str(), "on");
		return;
	}

	write_sysfs(i2c_path.c_str(), "auto");
}

static void add_i2c_callback(const std::string &d_name)
{
	class i2c_tunable *i2c;
	std::string filename;
	bool is_adapter = false;

	filename = std::format("/sys/bus/i2c/devices/{}/new_device", d_name);
	if (access(filename.c_str(), W_OK) == 0)
		is_adapter = true;

	filename = std::format("/sys/bus/i2c/devices/{}", d_name);
	i2c = new class i2c_tunable(filename, d_name, is_adapter);

	if (is_adapter)
		filename = std::format("/sys/bus/i2c/devices/{}/device", d_name);
	else
		filename = std::format("/sys/bus/i2c/devices/{}", d_name);

	if (device_has_runtime_pm(filename))
		all_tunables.push_back(i2c);
	else
		all_untunables.push_back(i2c);
}

void add_i2c_tunables(void)
{
	process_directory("/sys/bus/i2c/devices/", add_i2c_callback);
}
