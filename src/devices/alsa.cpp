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
#include <unistd.h>


using namespace std;

#include "device.h"
#include "alsa.h"
#include "../parameters/parameters.h"

#include "../devlist.h"

#include <string.h>
#include <unistd.h>

alsa::alsa(char *_name, char *path): device()
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

	sprintf(devname, "alsa:%s", _name);
	sprintf(humanname, "alsa:%s", _name);
	strncpy(name, devname, sizeof(name));
	rindex = get_result_index(name);

	guilty[0] = 0;
	model[0] = 0;
	vendor[0] = 0;
	sprintf(devname, "%s/modelname", path);
	file.open(devname);
	if (file) {
		file.getline(model, 4096);
		file.close();
	}
	sprintf(devname, "%s/vendor_name", path);
	file.open(devname);
	if (file) {
		file.getline(vendor, 4096);
		file.close();
	}
	if (strlen(model) && strlen(vendor))
		sprintf(humanname, _("Audio codec %s: %s (%s)"), name, model, vendor);
	else if (strlen(model))
		sprintf(humanname, _("Audio codec %s: %s"), _name, model);
	else if (strlen(vendor))
		sprintf(humanname, _("Audio codec %s: %s"), _name, vendor);
}

void alsa::start_measurement(void)
{
	char filename[4096];
	ifstream file;

	sprintf(filename, "%s/power_off_acct", sysfs_path);
	try {
		file.open(filename, ios::in);
		if (file) {
			file >> start_inactive;
		}
		file.close();
		sprintf(filename, "%s/power_on_acct", sysfs_path);
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
	char filename[4096];
	ifstream file;
	double p;

	sprintf(filename, "%s/power_off_acct", sysfs_path);
	try {
		file.open(filename, ios::in);
		if (file) {
			file >> end_inactive;
		}
		file.close();
		sprintf(filename, "%s/power_on_acct", sysfs_path);
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

void create_all_alsa(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];

	dir = opendir("/sys/class/sound/card0/");
	if (!dir)
		return;
	while (1) {
		class alsa *bl;
		ofstream file;
		entry = readdir(dir);
		if (!entry)
			break;
		if (strncmp(entry->d_name, "hwC", 3) != 0)
			continue;
		sprintf(filename, "/sys/class/sound/card0/%s/power_on_acct", entry->d_name);

		if (access(filename, R_OK) != 0)
			continue;

		sprintf(filename, "/sys/class/sound/card0/%s", entry->d_name);

		bl = new class alsa(entry->d_name, filename);
		all_devices.push_back(bl);
		register_parameter("alsa-codec-power", 0.5);
	}
	closedir(dir);

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
