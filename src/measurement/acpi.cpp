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
#include "acpi.h"
#include "../lib.h"
#include <format>

acpi_power_meter::acpi_power_meter(const std::string &acpi_name) : power_meter(acpi_name)
{
	rate = 0.0;
	capacity = 0.0;
	voltage = 0.0;
}

void acpi_power_meter::measure(void)
{
	rate = 0.0;
	voltage = 0.0;
	capacity = 0.0;

	std::string base = std::format("/sys/class/power_supply/{}/", name);

	std::string status = read_sysfs_string(base + "status");
	if (status != "Discharging")
		return;

	/* Try direct power_now (µW → W) */
	std::string pw = read_sysfs_string(base + "power_now");
	if (!pw.empty()) {
		try { rate = std::stod(pw) / 1000000.0; } catch (...) {}
		return;
	}

	/* Fall back to current_now (µA) × voltage_now (µV) → W */
	std::string cv = read_sysfs_string(base + "current_now");
	std::string vv = read_sysfs_string(base + "voltage_now");
	if (!cv.empty() && !vv.empty()) {
		try {
			rate = (std::stod(cv) * std::stod(vv)) / 1e12;
		} catch (...) {}
	}
}


void acpi_power_meter::end_measurement(void)
{
	measure();
}

void acpi_power_meter::start_measurement(void)
{
	/* ACPI battery state is a lagging indication, lets only measure at the end */
}


double acpi_power_meter::power(void)
{
	return rate;
}

void acpi_power_meter::collect_json_fields(std::string &_js)
{
    power_meter::collect_json_fields(_js);
    JSON_FIELD(capacity);
    JSON_FIELD(rate);
    JSON_FIELD(voltage);
}
