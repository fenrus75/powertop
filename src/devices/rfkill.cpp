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

rfkill::rfkill(char *_name, char *path): device()
{
	char line[4096];
	char filename[PATH_MAX];
	char devname[128];
	start_soft = 0;
	start_hard = 0;
	end_soft = 0;
	end_hard = 0;
	pt_strcpy(sysfs_path, path);
	register_sysfs_path(sysfs_path);
	snprintf(devname, sizeof(devname), "radio:%s", _name);
	snprintf(humanname, sizeof(humanname), "radio:%s", _name);
	pt_strcpy(name, devname);
	register_parameter(devname);
	index = get_param_index(devname);
	rindex = get_result_index(name);

	memset(line, 0, 4096);
	snprintf(filename, sizeof(filename), "%s/device/driver", path);
	if (readlink(filename, line, sizeof(line)) > 0) {
		snprintf(humanname, sizeof(humanname), _("Radio device: %s"), basename(line));
	}
	snprintf(filename, sizeof(filename), "%s/device/device/driver", path);
	if (readlink(filename, line, sizeof(line)) > 0) {
		snprintf(humanname, sizeof(humanname), _("Radio device: %s"), basename(line));
	}
}

void rfkill::start_measurement(void)
{
	char filename[PATH_MAX];
	ifstream file;

	start_hard = 1;
	start_soft = 1;
	end_hard = 1;
	end_soft = 1;

	snprintf(filename, sizeof(filename), "%s/hard", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> start_hard;
	}
	file.close();

	snprintf(filename, sizeof(filename), "%s/soft", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> start_soft;
	}
	file.close();
}

void rfkill::end_measurement(void)
{
	char filename[PATH_MAX];
	ifstream file;

	snprintf(filename, sizeof(filename), "%s/hard", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> end_hard;
	}
	file.close();
	snprintf(filename, sizeof(filename), "%s/soft", sysfs_path);
	file.open(filename, ios::in);
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

const char * rfkill::device_name(void)
{
	return name;
}

static void create_all_rfkills_callback(const char *d_name)
{
	char filename[PATH_MAX];
	char name[4096] = {0};
	class rfkill *bl;
	ifstream file;

	snprintf(filename, sizeof(filename), "/sys/class/rfkill/%s/name", d_name);
	strncpy(name, d_name, sizeof(name) - 1);
	file.open(filename, ios::in);
	if (file) {
		file.getline(name, 100);
		file.close();
	}

	snprintf(filename, sizeof(filename), "/sys/class/rfkill/%s", d_name);
	bl = new class rfkill(name, filename);
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
