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


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <format>

#include "../parameters/parameters.h"
#include "../lib.h"

#include <iostream>
#include <fstream>

runtime_pmdevice::runtime_pmdevice(const std::string &_name, const std::string &path) : device()
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
	before_suspended_time = read_sysfs(std::format("{}/power/runtime_suspended_time", sysfs_path));
	before_active_time = read_sysfs(std::format("{}/power/runtime_active_time", sysfs_path));

	after_suspended_time = 0;
	after_active_time = 0;
}

void runtime_pmdevice::end_measurement(void)
{
	after_suspended_time = read_sysfs(std::format("{}/power/runtime_suspended_time", sysfs_path));
	after_active_time = read_sysfs(std::format("{}/power/runtime_active_time", sysfs_path));
}

double runtime_pmdevice::utilization(void) /* percentage */
{
	double d;
	double delta_active = (double)after_active_time - before_active_time;
	double delta_suspended = (double)after_suspended_time - before_suspended_time;
	double total_time = delta_active + delta_suspended;

	if (total_time < 0.01)
		return 0.0;

	d = 100.0 * delta_active / total_time;

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

void runtime_pmdevice::set_human_name(const std::string &_name)
{
	humanname = _name;
}


bool device_has_runtime_pm(const std::string &sysfs_path)
{
	if (read_sysfs(std::format("{}/power/runtime_suspended_time", sysfs_path)))
		return true;

	if (read_sysfs(std::format("{}/power/runtime_active_time", sysfs_path)))
		return true;

	return false;
}

static void do_bus(const std::string &bus)
{
	std::string bus_path = std::format("/sys/bus/{}/devices/", bus);

	for (const auto &devname : list_directory(bus_path)) {
		std::string path = std::format("/sys/bus/{}/devices/{}", bus, devname);
		class runtime_pmdevice *dev = new runtime_pmdevice(devname, path);

		if (bus == "i2c") {
			std::string devnode;
			bool is_adapter = false;

			if (access(std::format("/sys/bus/{}/devices/{}/new_device", bus, devname).c_str(), W_OK) == 0)
				is_adapter = true;

			devnode = read_sysfs_string(std::format("/sys/bus/{}/devices/{}/name", bus, devname));

			dev->set_human_name(pt_format(_("I2C {} ({}): {}"), (is_adapter ? _("Adapter") : _("Device")), devname, devnode));
		}

		if (bus == "pci") {
			uint16_t vendor = 0, device = 0;
			std::string content;

			content = read_sysfs_string(std::format("/sys/bus/{}/devices/{}/vendor", bus, devname));
			if (!content.empty()) {
				try {
					vendor = std::stoul(content, nullptr, 16);
				} catch (...) {}
			}

			content = read_sysfs_string(std::format("/sys/bus/{}/devices/{}/device", bus, devname));
			if (!content.empty()) {
				try {
					device = std::stoul(content, nullptr, 16);
				} catch (...) {}
			}

			if (vendor && device) {
				dev->set_human_name(pt_format(_("PCI Device: {}"),
					pci_id_to_name(vendor, device)));
			}
		}

		if (!device_has_runtime_pm(path)) {
			delete dev;
			continue;
		}
		all_devices.push_back(dev);
	}
}

void create_all_runtime_pm_devices(void)
{
	do_bus("pci");
	do_bus("spi");
	do_bus("platform");
	do_bus("i2c");
}

void runtime_pmdevice::collect_json_fields(std::string &_js)
{
	device::collect_json_fields(_js);
	JSON_FIELD(before_suspended_time);
	JSON_FIELD(before_active_time);
	JSON_FIELD(after_suspended_time);
	JSON_FIELD(after_active_time);
	JSON_FIELD(sysfs_path);
	JSON_FIELD(name);
	JSON_FIELD(humanname);
	JSON_FIELD(index);
	JSON_FIELD(r_index);
}
