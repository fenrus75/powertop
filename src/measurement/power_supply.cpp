/*
 * Copyright 2011, Intel Corporation
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
 *	John Mathew <johnx.mathew@intel.com>
 */
#include "measurement.h"
#include "power_supply.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

power_supply::power_supply(const char *supply_name)
{
	rate = 0.0;
	capacity = 0.0;
	voltage = 0.0;
	strncpy(battery_name, supply_name, sizeof(battery_name));
}

/*
POWER_SUPPLY_NAME=msic-battery
POWER_SUPPLY_STATUS=Discharging
POWER_SUPPLY_HEALTH=Cold
POWER_SUPPLY_PRESENT=1
POWER_SUPPLY_TECHNOLOGY=Li-ion
POWER_SUPPLY_VOLTAGE_MAX_DESIGN=4200000
POWER_SUPPLY_VOLTAGE_NOW=4119000
POWER_SUPPLY_CURRENT_NOW=-290000
POWER_SUPPLY_CHARGE_NOW=1503000
POWER_SUPPLY_CHARGE_COUNTER=-254923
POWER_SUPPLY_CHARGE_FULL_DESIGN=1500000
POWER_SUPPLY_CHARGE_FULL=1500000
POWER_SUPPLY_CHARGE_AVG=32762000
POWER_SUPPLY_ENERGY_FULL=6300000
POWER_SUPPLY_ENERGY_NOW=6235000
POWER_SUPPLY_CAPACITY_LEVEL=Full
POWER_SUPPLY_CAPACITY=100
POWER_SUPPLY_TEMP=-340
POWER_SUPPLY_MODEL_NAME=CDK0
POWER_SUPPLY_MANUFACTURER=IN

Quoting include/linux/power_supply.h:

All voltages, currents, charges, energies, time and temperatures in µV,
µA, µAh, µWh, seconds and tenths of degree Celsius unless otherwise
stated. It's driver's job to convert its raw values to units in which
this class operates.
*/

void power_supply::measure(void)
{
	char filename[4096];
	char line[4096];
	ifstream file;

	double _power_rate = 0;
	double _current_rate = 0;
	double _capacity = 0;
	double _voltage = 0;

	rate = 0;
	voltage = 0;
	capacity = 0;

	sprintf(filename, "/sys/class/power_supply/%s/uevent", battery_name);

	file.open(filename, ios::in);
	if (!file)
		return;

	while (file) {
		char *c;
		file.getline(line, 4096);

		if (strstr(line, "PRESENT")) {
			c = strchr(line, '=');
			c++;
			if(*c == '0'){
				printf ("Battery not present");
				return;
			}
		}
		if (strstr(line, "CURRENT_NOW")) {
			c = strchr(line, '=');
			c++;
			if(*c == '-') c++; // ignoring the negative sign
			_current_rate = strtoull(c, NULL, 10);
			if (c) {
				//printf ("CURRENT: %f. \n",_current_rate);
			} else {
				_current_rate = 0;
			}
		}
		if (strstr(line, "POWER_NOW")) {
			c = strchr(line, '=');
			c++;
			_power_rate = strtoull(c, NULL, 10);
			if (c) {
				//printf ("POWER: %f. \n",_power_rate);
			} else {
				_power_rate = 0;
			}
		}
		if (strstr(line, "CAPACITY=")) {
			c = strchr(line, '=');
			c++;
			_capacity = strtoull(c, NULL, 10);
			if (c) {
				//printf ("CAPACITY: %f. \n",_capacity);
			} else {
				_capacity = 0;
			}
		}
		if (strstr(line, "VOLTAGE_NOW")) {
			c = strchr(line, '=');
			c++;
			while (*c == ' ') c++;
			_voltage = strtoull(c, NULL, 10);
			if (c) {
				//printf ("VOLTAGE_NOW: %f. \n",_voltage);
			} else {
				_voltage = 0;
			}
		}
	}
	file.close();

	if(_voltage) {
		_voltage = _voltage / 1000.0;
		voltage = _voltage;
	} else {
		voltage = 0.0;
	}

	if(_power_rate)
	{
		rate = _power_rate / 1000000.0;
	}
	else if(_current_rate) {
		_current_rate = _current_rate / 1000.0;
		rate = _current_rate * _voltage;
	} else {
		rate = 0.0;
	}

	if(_capacity)
		capacity = _capacity;
	else
		capacity = 0.0;
}


void power_supply::end_measurement(void)
{
	measure();
}

void power_supply::start_measurement(void)
{
	/* ACPI battery state is a lagging indication, lets only measure at the end */
}


double power_supply::joules_consumed(void)
{
	return rate;
}
