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
#include <limits.h>
#include <format>

#include "../parameters/parameters.h"
#include "../lib.h"

#include <iostream>
#include <fstream>

runtime_pmdevice::runtime_pmdevice(const string &_name, const string &path) : device()
{
	sysfs_path = path;
	register_sysfs_path(sysfs_path);
	name = _name;
	humanname = std::format("runtime-{}", _name);

	index = get_param_index(humanname);
	r_index = get_result_index(humanname);

	before_suspended_time = 0;
	before_active_time = 0;
	after_suspended_time = 0;
	after_active_time = 0;
	register_parameter(humanname);
}

void runtime_pmdevice::start_measurement(void)
{
	ifstream file;

	before_suspended_time = 0;
	before_active_time = 0;
        after_suspended_time = 0;
	after_active_time = 0;

	file.open(std::format("{}/power/runtime_suspended_time", sysfs_path), ios::in);
	if (!file)
		return;
	file >> before_suspended_time;
	file.close();

	file.open(std::format("{}/power/runtime_active_time", sysfs_path), ios::in);
	if (!file)
		return;
	file >> before_active_time;
	file.close();
}

void runtime_pmdevice::end_measurement(void)
{
	ifstream file;

	file.open(std::format("{}/power/runtime_suspended_time", sysfs_path), ios::in);
	if (!file)
		return;
	file >> after_suspended_time;
	file.close();

	file.open(std::format("{}/power/runtime_active_time", sysfs_path), ios::in);
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

void runtime_pmdevice::set_human_name(const string &_name)
{
	humanname = _name;
}


bool device_has_runtime_pm(const string &sysfs_path)
{
	ifstream file;
	unsigned long value;

	file.open(std::format("{}/power/runtime_suspended_time", sysfs_path), ios::in);
	if (file) {
		file >> value;
		file.close();
		if (value)
			return true;
	}

	file.open(std::format("{}/power/runtime_active_time", sysfs_path), ios::in);
	if (file) {
		file >> value;
		file.close();
		if (value)
			return true;
	}

	return false;
}

static void do_bus(const char *bus)
{
	/* /sys/bus/pci/devices/0000\:00\:1f.0/power/runtime_suspended_time */

	struct dirent *entry;
	DIR *dir;

	dir = opendir(std::format("/sys/bus/{}/devices/", bus).c_str());
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

		dev = new class runtime_pmdevice(entry->d_name, std::format("/sys/bus/{}/devices/{}", bus, entry->d_name));
		if (strcmp(bus, "i2c") == 0) {
			std::string devname;
			bool is_adapter = false;

			if (access(std::format("/sys/bus/{}/devices/{}/new_device", bus, entry->d_name).c_str(), W_OK) == 0)
				is_adapter = true;

			file.open(std::format("/sys/bus/{}/devices/{}/name", bus, entry->d_name), ios::in);
			if (file) {
				getline(file, devname);
				file.close();
			}

			dev->set_human_name(pt_format(_("I2C {} ({}): {}"), (is_adapter ? _("Adapter") : _("Device")), entry->d_name, devname));
		}

		if (strcmp(bus, "pci") == 0) {
			uint16_t vendor = 0, device = 0;

			file.open(std::format("/sys/bus/{}/devices/{}/vendor", bus, entry->d_name), ios::in);
			if (file) {
				file >> hex >> vendor;
				file.close();
			}


			file.open(std::format("/sys/bus/{}/devices/{}/device", bus, entry->d_name), ios::in);
			if (file) {
				file >> hex >> device;
				file.close();
			}

			if (vendor && device) {
				dev->set_human_name(pt_format(_("PCI Device: {}"),
					pci_id_to_name(vendor, device)));
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
