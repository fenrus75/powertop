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
#include <libgen.h>
#include <unistd.h>
#include <limits.h>


using namespace std;

#include "device.h"
#include "rfkill.h"
#include "../parameters/parameters.h"

#include <string.h>
#include <unistd.h>
#include <format>

rfkill::rfkill(const string &_name, const string &path): device()
{
	char line[4096];
	start_soft = 0;
	start_hard = 0;
	end_soft = 0;
	end_hard = 0;
	sysfs_path = path;
	register_sysfs_path(sysfs_path);
	name = std::format("radio:{}", _name);
	humanname = std::format("radio:{}", _name);
	register_parameter(name);
	index = get_param_index(name);
	rindex = get_result_index(name);

	memset(line, 0, 4096);
	if (readlink(std::format("{}/device/driver", path).c_str(), line, sizeof(line)) > 0) {
		humanname = pt_format(_("Radio device: {}"), basename(line));
	}
	if (readlink(std::format("{}/device/device/driver", path).c_str(), line, sizeof(line)) > 0) {
		humanname = pt_format(_("Radio device: {}"), basename(line));
	}
}

void rfkill::start_measurement(void)
{
	ifstream file;

	start_hard = 1;
	start_soft = 1;
	end_hard = 1;
	end_soft = 1;

	file.open(std::format("{}/hard", sysfs_path), ios::in);
	if (file) {
		file >> start_hard;
	}
	file.close();

	file.open(std::format("{}/soft", sysfs_path), ios::in);
	if (file) {
		file >> start_soft;
	}
	file.close();
}

void rfkill::end_measurement(void)
{
	ifstream file;

	file.open(std::format("{}/hard", sysfs_path), ios::in);
	if (file) {
		file >> end_hard;
	}
	file.close();
	file.open(std::format("{}/soft", sysfs_path), ios::in);
	if (file) {
		file >> end_soft;
	}
	file.close();

	report_utilization(name, utilization());
}


double rfkill::utilization(void)
{
	double p;
	int rfk;

	rfk = start_soft+end_soft;
	if (rfk <  start_hard+end_hard)
		rfk = start_hard+end_hard;

	p = 100 - 50.0 * rfk;

	return p;
}

static void create_all_rfkills_callback(const std::string &d_name)
{
	std::string name(d_name);
	class rfkill *bl;
	ifstream file;

	file.open(std::format("/sys/class/rfkill/{}/name", d_name), ios::in);
	if (file) {
		getline(file, name);
		file.close();
	}

	bl = new class rfkill(name, std::format("/sys/class/rfkill/{}", d_name));
	all_devices.push_back(bl);
}

void create_all_rfkills(void)
{
	process_directory("/sys/class/rfkill/", create_all_rfkills_callback);
}

double rfkill::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;

	power = 0;
	factor = get_parameter_value(index, bundle);
	util = get_result_value(rindex, result);

	power += util * factor / 100.0;

	return power;
}
