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
#pragma once

#include "measurement.h"

class sysfs_power_meter: public power_meter {
	double capacity = 0.0;
	double rate = 0.0;

	bool get_sysfs_attr(const std::string &attribute, int *value);
	bool is_present();
	double get_voltage();

	bool set_rate_from_power();
	bool set_rate_from_current(double voltage);
	bool set_capacity_from_energy();
	bool set_capacity_from_charge(double voltage);

	void measure();
public:
	sysfs_power_meter(const std::string &power_supply_name);
	virtual void start_measurement(void) override;
	virtual void end_measurement(void) override;

	virtual double power(void) override { return rate; }
	virtual double dev_capacity(void) override { return capacity; }
	void collect_json_fields(std::string &_js) override;
};

