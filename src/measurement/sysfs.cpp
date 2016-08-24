/*
 * Copyright (c) 2011 Anssi Hannula
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
 *	Anssi Hannula <anssi.hannula@iki.fi>
 */
#include "measurement.h"
#include "sysfs.h"
#include "../lib.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>

sysfs_power_meter::sysfs_power_meter(const char *power_supply_name)
{
	rate = 0.0;
	capacity = 0.0;
	pt_strcpy(name, power_supply_name);
}

bool sysfs_power_meter::get_sysfs_attr(const char *attribute, int *value)
{
	char filename[PATH_MAX];
	bool ok;

	snprintf(filename, sizeof(filename), "/sys/class/power_supply/%s/%s", name, attribute);
	*value = read_sysfs(filename, &ok);

	return ok;
}

bool sysfs_power_meter::is_present()
{
	int present = 0;

	if (!get_sysfs_attr("present", &present))
		return true; /* assume always present */

	return present;
}

double sysfs_power_meter::get_voltage()
{
	int voltage;

	if (!get_sysfs_attr("voltage_now", &voltage))
		return -1.0;

	/* µV to V */
	return voltage / 1000000.0;
}

bool sysfs_power_meter::set_rate_from_power()
{
	int power;

	if (!get_sysfs_attr("power_now", &power))
		return false;

	/* µW to W */
	rate = power / 1000000.0;
	return true;
}

bool sysfs_power_meter::set_rate_from_current(double voltage)
{
	int current;

	if (!get_sysfs_attr("current_now", &current))
		return false;

	/* current: µA
	 * voltage: V
	 * rate: W */
	rate = (current / 1000000.0) * voltage;
	return true;
}

bool sysfs_power_meter::set_capacity_from_energy()
{
	int energy;

	if (!get_sysfs_attr("energy_now", &energy))
		return false;

	/* µWh to J */
	capacity = energy / 1000000.0 * 3600.0;
	return true;
}

bool sysfs_power_meter::set_capacity_from_charge(double voltage)
{
	int charge;

	if (!get_sysfs_attr("charge_now", &charge))
		return false;

	/* charge: µAh
	 * voltage: V
	 * capacity: J */
	capacity = (charge / 1000000.0) * voltage * 3600.0;
	return true;
}

void sysfs_power_meter::measure()
{
	bool got_rate = false;
	bool got_capacity = false;

	rate = 0.0;
	capacity = 0.0;
	this->set_discharging(false);

	if (!is_present())
		return;
	/** do not jump over. we may have discharging battery */
	if (read_sysfs_string("/sys/class/power_supply/%s/status", name) == "Discharging")
		this->set_discharging(true);

	got_rate = set_rate_from_power();
	got_capacity = set_capacity_from_energy();

	if (!got_rate || !got_capacity) {
		double voltage = get_voltage();
		if (voltage < 0.0)
			return;
		if (!got_rate)
			set_rate_from_current(voltage);
		if (!got_capacity)
			set_capacity_from_charge(voltage);
	}
}

void sysfs_power_meter::end_measurement(void)
{
	measure();
}

void sysfs_power_meter::start_measurement(void)
{
	/* Battery state is generally a lagging indication, lets only measure at the end */
}
