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


using namespace std;

#include "device.h"
#include "rfkill.h"
#include "../parameters/parameters.h"

#include <string.h>
#include <unistd.h>

rfkill::rfkill(char *_name, char *path): device()
{
	char line[4096];
	char filename[4096];
	char devname[128];
	start_soft = 0;
	start_hard = 0;
	end_soft = 0;
	end_hard = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));
	register_sysfs_path(sysfs_path);
	sprintf(devname, "radio:%s", _name);
	sprintf(humanname, "radio:%s", _name);
	strncpy(name, devname, sizeof(name));
	register_parameter(devname);
	index = get_param_index(devname);
	rindex = get_result_index(name);

	memset(line, 0, 4096);
	sprintf(filename, "%s/device/driver", path);
	if (readlink(filename, line, 4096) > 0) {
		sprintf(humanname, _("Radio device: %s"), basename(line));
	}
	sprintf(filename, "%s/device/device/driver", path);
	if (readlink(filename, line, 4096) > 0) {
		sprintf(humanname, _("Radio device: %s"), basename(line));
	}
}

void rfkill::start_measurement(void)
{
	char filename[4096];
	ifstream file;

	start_hard = 1;
	start_soft = 1;
	end_hard = 1;
	end_soft = 1;

	sprintf(filename, "%s/hard", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> start_hard;
	}
	file.close();

	sprintf(filename, "%s/soft", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> start_soft;
	}
	file.close();
}

void rfkill::end_measurement(void)
{
	char filename[4096];
	ifstream file;

	sprintf(filename, "%s/hard", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> end_hard;
	}
	file.close();
	sprintf(filename, "%s/soft", sysfs_path);
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
	char filename[4096];
	char name[4096];
	class rfkill *bl;
	ifstream file;

	sprintf(filename, "/sys/class/rfkill/%s/name", d_name);
	strcpy(name, d_name);
	file.open(filename, ios::in);
	if (file) {
		file.getline(name, 100);
		file.close();
	}

	sprintf(filename, "/sys/class/rfkill/%s", d_name);
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
