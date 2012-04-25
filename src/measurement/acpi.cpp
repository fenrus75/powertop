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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

acpi_power_meter::acpi_power_meter(const char *acpi_name)
{
	rate = 0.0;
	capacity = 0.0;
	voltage = 0.0;
	strncpy(battery_name, acpi_name, sizeof(battery_name));
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
	char filename[4096];
	char line[4096];
	ifstream file;

	double _rate = 0;
	double _capacity = 0;
	double _voltage = 0;

	char rate_units[16];
	char capacity_units[16];
	char voltage_units[16];

	rate_units[0] = 0;
	capacity_units[0] = 0;
	voltage_units[0] = 0;

	rate = 0;
	voltage = 0;
	capacity = 0;

	sprintf(filename, "/proc/acpi/battery/%s/state", battery_name);

	file.open(filename, ios::in);
	if (!file)
		return;

	while (file) {
		char *c;
		file.getline(line, 4096);

		if (strstr(line, "present:") && (strstr(line, "yes") == NULL)) {
			return;
		}
		if (strstr(line, "charging state:") && (strstr(line, "discharging") == NULL)) {
			return; /* not discharging */
		}
		if (strstr(line, "present rate:")) {
			c = strchr(line, ':');
			c++;
			while (*c == ' ') c++;
			_rate = strtoull(c, NULL, 10);
			c = strchr(c, ' ');
			if (c) {
				c++;
				strcpy(rate_units, c);
			} else {
				_rate = 0;
				strcpy(rate_units, "Unknown");
			}

		}
		if (strstr(line, "remaining capacity:")) {
			c = strchr(line, ':');
			c++;
			while (*c == ' ') c++;
			_capacity = strtoull(c, NULL, 10);
			c = strchr(c, ' ');
			if (c) {
				c++;
				strcpy(capacity_units, c);
			} else {
				_capacity = 0;
				strcpy(capacity_units, "Unknown");
			}
		}
		if (strstr(line, "present voltage:")) {
			c = strchr(line, ':');
			c++;
			while (*c == ' ') c++;
			_voltage = strtoull(c, NULL, 10);
			c = strchr(c, ' ');
			if (c) {
				c++;
				strcpy(voltage_units, c);
			} else {
				_voltage = 0;
				strcpy(voltage_units, "Unknown");
			}
		}
	}
	file.close();

	/* BIOS report random crack-inspired units. Lets try to get to the Si-system units */

	if (strcmp(voltage_units, "mV") == 0) {
		_voltage = _voltage / 1000.0;
		strcpy(voltage_units, "V");
	}

	if (strcmp(rate_units, "mW") == 0) {
		_rate = _rate / 1000.0;
		strcpy(rate_units, "W");
	}

	if (strcmp(rate_units, "mA") == 0) {
		_rate = _rate / 1000.0;
		strcpy(rate_units, "A");
	}

	if (strcmp(capacity_units, "mAh") == 0) {
		_capacity = _capacity / 1000.0;
		strcpy(capacity_units, "Ah");
	}
	if (strcmp(capacity_units, "mWh") == 0) {
		_capacity = _capacity / 1000.0;
		strcpy(capacity_units, "Wh");
	}
	if (strcmp(capacity_units, "Wh") == 0) {
		_capacity = _capacity * 3600.0;
		strcpy(capacity_units, "J");
	}


	if (strcmp(capacity_units, "Ah") == 0 && strcmp(voltage_units, "V") == 0) {
		_capacity = _capacity * 3600.0 * _voltage;
		strcpy(capacity_units, "J");
	}

	if (strcmp(rate_units, "A") == 0 && strcmp(voltage_units, "V")==0 ) {
		_rate = _rate * _voltage;
		strcpy(rate_units, "W");
	}




	if (strcmp(capacity_units, "J") == 0)
		capacity = _capacity;
	else
		capacity = 0.0;

	if (strcmp(rate_units, "W")==0)
		rate = _rate;
	else
		rate = 0.0;

	if (strcmp(voltage_units, "V")==0)
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


double acpi_power_meter::joules_consumed(void)
{
	return rate;
}
