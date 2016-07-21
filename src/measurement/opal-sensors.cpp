/*
 * Copyright (c) 2015 IBM Corp.
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
 *	Stewart Smith <stewart@linux.vnet.ibm.com>
 */
#include "measurement.h"
#include "opal-sensors.h"
#include "../lib.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>

opal_sensors_power_meter::opal_sensors_power_meter(const char *power_supply_name)
{
	strncpy(name, power_supply_name, sizeof(name));
}

double opal_sensors_power_meter::power(void)
{
	bool ok;
	int value;
	double r = 0;

	value = read_sysfs(name, &ok);

	if(ok)
		r = value / 1000000.0;
	return r;
}
