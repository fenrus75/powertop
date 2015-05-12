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
#include <unistd.h>

using namespace std;

#include "device.h"
#include "alsa.h"
#include "../parameters/parameters.h"

#include "../devlist.h"

#include <string.h>
#include <unistd.h>

alsa::alsa(const char *_name, const char *path): device()
{
	ifstream file;

	char devname[4096];
	char model[4096];
	char vendor[4096];
	end_active = 0;
	start_active = 0;
	end_inactive = 0;
	start_inactive = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));

	snprintf(devname, 4096, "alsa:%s", _name);
	snprintf(humanname, 4096, "alsa:%s", _name);
	strncpy(name, devname, sizeof(name));
	rindex = get_result_index(name);

	guilty[0] = 0;
	model[0] = 0;
	vendor[0] = 0;
	snprintf(devname, 4096, "%s/modelname", path);
	file.open(devname);
	if (file) {
		file.getline(model, 4096);
		file.close();
	}
	snprintf(devname, 4096, "%s/vendor_name", path);
	file.open(devname);
	if (file) {
		file.getline(vendor, 4096);
		file.close();
	}
	if (strlen(model) && strlen(vendor))
		snprintf(humanname, 4096, _("Audio codec %s: %s (%s)"), name, model, vendor);
	else if (strlen(model))
		snprintf(humanname, 4096, _("Audio codec %s: %s"), _name, model);
	else if (strlen(vendor))
		snprintf(humanname, 4096, _("Audio codec %s: %s"), _name, vendor);
}

void alsa::start_measurement(void)
{
	char filename[PATH_MAX];
	ifstream file;

	snprintf(filename, PATH_MAX, "%s/power_off_acct", sysfs_path);
	try {
		file.open(filename, ios::in);
		if (file) {
			file >> start_inactive;
		}
		file.close();
		snprintf(filename, PATH_MAX, "%s/power_on_acct", sysfs_path);
		file.open(filename, ios::in);

		if (file) {
			file >> start_active;
		}
		file.close();
	}
	catch (std::ios_base::failure &c) {
		fprintf(stderr, "%s\n", c.what());
	}
}

void alsa::end_measurement(void)
{
	char filename[PATH_MAX];
	ifstream file;
	double p;

	snprintf(filename, PATH_MAX, "%s/power_off_acct", sysfs_path);
	try {
		file.open(filename, ios::in);
		if (file) {
			file >> end_inactive;
		}
		file.close();
		snprintf(filename, PATH_MAX, "%s/power_on_acct", sysfs_path);
		file.open(filename, ios::in);

		if (file) {
			file >> end_active;
		}
		file.close();
	}
	catch (std::ios_base::failure &c) {
		fprintf(stderr, "%s\n", c.what());
	}

	p = (end_active - start_active) / (0.001 + end_active + end_inactive - start_active - start_inactive) * 100.0;
	report_utilization(name, p);
}


double alsa::utilization(void)
{
	double p;

	p = (end_active - start_active) / (0.001 + end_active - start_active + end_inactive - start_inactive) * 100.0;

	return p;
}

const char * alsa::device_name(void)
{
	return name;
}

static void create_all_alsa_callback(const char *d_name)
{
	char filename[PATH_MAX];
	class alsa *bl;

	if (strncmp(d_name, "hwC", 3) != 0)
		return;

	snprintf(filename, PATH_MAX, "/sys/class/sound/card0/%s/power_on_acct", d_name);
	if (access(filename, R_OK) != 0)
		return;

	snprintf(filename, PATH_MAX, "/sys/class/sound/card0/%s", d_name);
	bl = new class alsa(d_name, filename);
	all_devices.push_back(bl);
	register_parameter("alsa-codec-power", 0.5);
}

void create_all_alsa(void)
{
	process_directory("/sys/class/sound/card0/", create_all_alsa_callback);
}

double alsa::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;
	static int index = 0;

	power = 0;
	if (!index)
		index = get_param_index("alsa-codec-power");

	factor = get_parameter_value(index, bundle);

	util = get_result_value(rindex, result);

	power += util * factor / 100.0;

	return power;
}

void alsa::register_power_with_devlist(struct result_bundle *results, struct parameter_bundle *bundle)
{
	register_devpower(&name[7], power_usage(results, bundle), this);
}

const char * alsa::human_name(void)
{
	sprintf(temp_buf, "%s", humanname);
	if (strlen(guilty) > 0)
		sprintf(temp_buf, "%s (%s)", humanname, guilty);
	return temp_buf;
}
