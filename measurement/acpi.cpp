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

	sprintf(filename, "/proc/acpi/battery/%s/state", battery_name);

	file.open(filename, ios::in);
	if (!file)
		return;

	while (file) {
		char *c;
		file.getline(line, 4096);

		if (strstr(line, "present:") && strstr(line, "yes") != NULL)
			return; /* non present battery */
		if (strstr(line, "charging state:") && strstr(line, "discharging") != NULL)
			return; /* not discharging */
		if (strstr(line, "present rate:")) {
			c = strchr(line, ':');
			c++;
			while (*c == ' ') c++;
			_rate = strtoull(c, NULL, 10);
			c = strchr(c, ' ');
			c++;
			strcpy(rate_units, c);

		}
		if (strstr(line, "remaining capacity:")) {
			c = strchr(line, ':');
			c++;
			while (*c == ' ') c++;
			_capacity = strtoull(c, NULL, 10);
			c = strchr(c, ' ');
			c++;
			strcpy(capacity_units, c);
		}
		if (strstr(line, "present voltage:")) {
			c = strchr(line, ':');
			c++;
			while (*c == ' ') c++;
			_voltage = strtoull(c, NULL, 10);
			c = strchr(c, ' ');
			c++;
			strcpy(voltage_units, c);
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


	if (strcmp(capacity_units, "C") == 0)
		capacity = _capacity;
	else
		capacity = 0.0;

	if (strcmp(rate_units, "W")==0)
		rate = _rate;
	else
		rate = 0.0;
}


void acpi_power_meter::end_measurement(void)
{
	measure();
}

void acpi_power_meter::start_measurement(void)
{
}


double acpi_power_meter::joules_consumed(void)
{
	return 0.0;
}
