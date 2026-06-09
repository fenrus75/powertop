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

#include "measurement.h"
#include <filesystem>
#include <system_error>
#include "acpi.h"
#include "extech.h"
#include "sysfs.h"
#include "opal-sensors.h"
#include "../parameters/parameters.h"
#include "../lib.h"

#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <ctime>
#include <format>

static struct timespec tlast;

void start_power_measurement(void)
{
	clock_gettime(CLOCK_MONOTONIC, &tlast);
	for (auto &m : power_meters)
		m->start_measurement();
	all_results.joules = 0.0;
}

void end_power_measurement(void)
{
	for (auto &m : power_meters)
		m->end_measurement();
}

double global_power(void)
{
	bool global_discharging = false;
	double total = 0.0;

	for (auto &m : power_meters) {
		global_discharging |= m->is_discharging();
		total += m->power();
	}

	/* report global time left if at least one battery is discharging */
	if (!global_discharging)
		return 0.0;

	all_results.power = total;
	if (total < min_power && total > 0.01)
		min_power = total;
	return total;
}

void global_sample_power(void)
{
	struct timespec tnow;

	clock_gettime(CLOCK_MONOTONIC, &tnow);
	/* power * time = joules */
	all_results.joules += global_power() *
			( ((double)tnow.tv_sec + 1.0e-9*tnow.tv_nsec) -
			  ((double)tlast.tv_sec + 1.0e-9*tlast.tv_nsec));
	tlast = tnow;
}

double global_joules(void)
{
	return all_results.joules;
}

double global_time_left(void)
{
	bool global_discharging = false;
	double total_capacity = 0.0;
	double total_rate = 0.0;

	for (auto &m : power_meters) {
		global_discharging |= m->is_discharging();
		total_capacity += m->dev_capacity();
		total_rate += m->power();
	}

	/* report global time left if at least one battery is discharging */
	if (!global_discharging)
		return 0.0;

	/* return 0.0 instead of INF+ */
	if (total_rate < 0.001)
		return 0.0;
	return total_capacity / total_rate;
}

void sysfs_power_meters_callback(const std::string &d_name)
{
	const std::string type = read_sysfs_string(
		std::format("/sys/class/power_supply/{}/type", d_name));

	if (type != "Battery" && type != "UPS")
		return;

	power_meters.push_back(std::make_unique<sysfs_power_meter>(d_name));
}

void acpi_power_meters_callback(const std::string &d_name)
{
	power_meters.push_back(std::make_unique<acpi_power_meter>(d_name));
}

void sysfs_opal_sensors_callback(const std::string &d_name)
{
	/* Those that end in / are directories and we don't want them */
	if (!d_name.empty() && d_name.back() == '/')
		return;

	power_meters.push_back(std::make_unique<opal_sensors_power_meter>(d_name));
}

void detect_power_meters(void)
{
	process_directory("/sys/class/power_supply", sysfs_power_meters_callback);
	
	std::error_code ec;
	const std::string hwmon_base = "/sys/devices/platform/opal-sensor/hwmon/";
	
	if (std::filesystem::exists(hwmon_base, ec)) {
		for (const auto& hwmon_entry : std::filesystem::directory_iterator(hwmon_base, ec)) {
			if (!hwmon_entry.is_directory(ec)) continue;
			const std::string hwmon_name = hwmon_entry.path().filename().string();
			if (!hwmon_name.starts_with("hwmon")) continue;
			
			for (const auto& power_entry : std::filesystem::directory_iterator(hwmon_entry.path(), ec)) {
				const std::string power_name = power_entry.path().filename().string();
				if (power_name.starts_with("power")) {
					if (power_entry.is_directory(ec)) continue;
					sysfs_opal_sensors_callback(power_entry.path().string());
				}
			}
		}
	}
}

void extech_power_meter(const std::string &devnode)
{
	power_meters.emplace_back(new class extech_power_meter(devnode));
}

void clear_power_meters(void)
{
	power_meters.clear();
}
