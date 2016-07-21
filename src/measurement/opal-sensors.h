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
#ifndef INCLUDE_GUARD_OPAL_SENSORS_H
#define INCLUDE_GUARD_OPAL_SENSORS_H

#include "measurement.h"
#include <limits.h>

class opal_sensors_power_meter: public power_meter {
	char name[PATH_MAX];
public:
	opal_sensors_power_meter(const char *power_supply_name);
	virtual void start_measurement(void) {};
	virtual void end_measurement(void) {};

	virtual double power(void);
	virtual double dev_capacity(void) { return 0.0; }
};

#endif
