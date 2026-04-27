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
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "../lib.h"

using namespace std;

acpi_power_meter::acpi_power_meter(const string &acpi_name) : power_meter(acpi_name)
{
	rate = 0.0;
	capacity = 0.0;
	voltage = 0.0;
}

/*
present:                 yes
capacity state:          ok
charging state:          discharging
present rate:            8580 mW
remaining capacity:      34110 mWh
present voltage:         12001 mV
*/

void acpi_power_meter::measure(void)
{
	std::string filename;
	std::string content;
	std::istringstream stream;
	std::string line;

	double _rate = 0;
	double _capacity = 0;
	double _voltage = 0;

	std::string rate_units = "Unknown";
	std::string capacity_units = "Unknown";
	std::string voltage_units = "Unknown";


	rate = 0;
	voltage = 0;
	capacity = 0;

	filename = std::format("/proc/acpi/battery/{}/state", name);
	content = read_file_content(filename);
	if (content.empty())
		return;

	stream.str(content);

	while (std::getline(stream, line)) {
		if (line.find("present:") != std::string::npos && line.find("yes") == std::string::npos) {
			return;
		}
		if (line.find("charging state:") != std::string::npos && line.find("discharging") == std::string::npos) {
			return; /* not discharging */
		}
		if (line.find("present rate:") != std::string::npos) {
			size_t pos = line.find(':');
			if (pos != std::string::npos) {
				std::istringstream iss(line.substr(pos + 1));
				iss >> _rate >> rate_units;
			}
		}
		if (line.find("remaining capacity:") != std::string::npos) {
			size_t pos = line.find(':');
			if (pos != std::string::npos) {
				std::istringstream iss(line.substr(pos + 1));
				iss >> _capacity >> capacity_units;
			}
		}
		if (line.find("present voltage:") != std::string::npos) {
			size_t pos = line.find(':');
			if (pos != std::string::npos) {
				std::istringstream iss(line.substr(pos + 1));
				iss >> _voltage >> voltage_units;
			}
		}
	}

	/* BIOS report random crack-inspired units. Lets try to get to the Si-system units */

	if (voltage_units == "mV") {
		_voltage = _voltage / 1000.0;
		voltage_units = "V";
	}

	if (rate_units == "mW") {
		_rate = _rate / 1000.0;
		rate_units = "W";
	}

	if (rate_units == "mA") {
		_rate = _rate / 1000.0;
		rate_units = "A";
	}

	if (capacity_units == "mAh") {
		_capacity = _capacity / 1000.0;
		capacity_units = "Ah";
	}
	if (capacity_units == "mWh") {
		_capacity = _capacity / 1000.0;
		capacity_units = "Wh";
	}
	if (capacity_units == "Wh") {
		_capacity = _capacity * 3600.0;
		capacity_units = "J";
	}


	if (capacity_units == "Ah" && voltage_units == "V") {
		_capacity = _capacity * 3600.0 * _voltage;
		capacity_units = "J";
	}

	if (rate_units == "A" && voltage_units == "V") {
		_rate = _rate * _voltage;
		rate_units = "W";
	}




	if (capacity_units == "J")
		capacity = _capacity;
	else
		capacity = 0.0;

	if (rate_units == "W")
		rate = _rate;
	else
		rate = 0.0;

	if (voltage_units == "V")
		voltage = _voltage;
	else
		voltage = 0.0;
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
