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
#include "runtime_pm.h"

#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "../parameters/parameters.h"
#include "../lib.h"

#include <iostream>
#include <fstream>

runtime_pmdevice::runtime_pmdevice(const char *_name, const char *path) : device()
{
	strcpy(sysfs_path, path);
	register_sysfs_path(sysfs_path);
	strcpy(name, _name);
	sprintf(humanname, "runtime-%s", _name);

	index = get_param_index(humanname);
	r_index = get_result_index(humanname);

	before_suspended_time = 0;
	before_active_time = 0;
        after_suspended_time = 0;
	after_active_time = 0;
}



void runtime_pmdevice::start_measurement(void)
{
	char filename[4096];
	ifstream file;

	before_suspended_time = 0;
	before_active_time = 0;
        after_suspended_time = 0;
	after_active_time = 0;

	sprintf(filename, "%s/power/runtime_suspended_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return;
	file >> before_suspended_time;
	file.close();

	sprintf(filename, "%s/power/runtime_active_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return;
	file >> before_active_time;
	file.close();
}

void runtime_pmdevice::end_measurement(void)
{
	char filename[4096];
	ifstream file;

	sprintf(filename, "%s/power/runtime_suspended_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return;
	file >> after_suspended_time;
	file.close();

	sprintf(filename, "%s/power/runtime_active_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return;
	file >> after_active_time;
	file.close();
}

double runtime_pmdevice::utilization(void) /* percentage */
{
	double d;
	d = 100 * (after_active_time - before_active_time) / (0.0001 + after_active_time - before_active_time + after_suspended_time - before_suspended_time);

	if (d < 0.00)
		d = 0.0;
	if (d > 99.9)
		d = 100.0;
	return d;
}

const char * runtime_pmdevice::device_name(void)
{
	return name;
}

const char * runtime_pmdevice::human_name(void)
{
	return humanname;
}


double runtime_pmdevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;

	power = 0;

	factor = get_parameter_value(index, bundle);
	util = get_result_value(r_index, result);
        power += util * factor / 100.0;

	return power;
}

void runtime_pmdevice::set_human_name(char *_name)
{
	strcpy(humanname, _name);
}


int device_has_runtime_pm(const char *sysfs_path)
{
	char filename[4096];
	ifstream file;
	unsigned long value;

	sprintf(filename, "%s/power/runtime_suspended_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return 0;
	file >> value;
	file.close();
	if (value)
		return 1;

	sprintf(filename, "%s/power/runtime_active_time", sysfs_path);
	file.open(filename, ios::in);
	if (!file)
		return 0;
	file >> value;
	file.close();
	if (value)
		return 1;

	return 0;
}


static void do_bus(const char *bus)
{
	/* /sys/bus/pci/devices/0000\:00\:1f.0/power/runtime_suspended_time */

	struct dirent *entry;
	DIR *dir;
	char filename[4096];

	sprintf(filename, "/sys/bus/%s/devices/", bus);
	dir = opendir(filename);
	if (!dir)
		return;
	while (1) {
		ifstream file;
		class runtime_pmdevice *dev;
		entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		sprintf(filename, "/sys/bus/%s/devices/%s", bus, entry->d_name);

		if (!device_has_runtime_pm(filename))
			continue;

		dev = new class runtime_pmdevice(entry->d_name, filename);

		if (strcmp(bus, "pci") == 0) {
			uint16_t vendor = 0, device = 0;

			sprintf(filename, "/sys/bus/%s/devices/%s/vendor", bus, entry->d_name);

			file.open(filename, ios::in);
			if (file) {
				file >> hex >> vendor;
				file.close();
			}


			sprintf(filename, "/sys/bus/%s/devices/%s/device", bus, entry->d_name);
			file.open(filename, ios::in);
			if (file) {
				file >> hex >> device;
				file.close();
			}

			if (vendor && device) {
				char devname[4096];
				sprintf(devname, _("PCI Device: %s"), pci_id_to_name(vendor, device, filename, 4095));
				dev->set_human_name(devname);
			}
		}
		all_devices.push_back(dev);
	}
	closedir(dir);
}

void create_all_runtime_pm_devices(void)
{
	do_bus("pci");
	do_bus("spi");
	do_bus("platform");
	do_bus("i2c");
}