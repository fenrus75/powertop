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
#pragma once

#include <vector>
#include <string>
#include "../lib.h"

class power_meter {
	bool discharging = false;
public:
	std::string name;
	power_meter(const std::string &n) : name(n) {}
	power_meter() : name("") {}
	virtual ~power_meter() {};

	virtual void start_measurement(void);
	virtual void end_measurement(void);
	virtual double power(void);

	virtual double dev_capacity(void)
	{
		return 0.0; /* in Joules */
	}

	virtual void set_discharging(bool d)
	{
		discharging = d;
	}

	virtual bool is_discharging()
	{
		return discharging;
	}

	virtual void collect_json_fields(std::string &_js);
	std::string serialize() { JSON_START(); collect_json_fields(_js); JSON_END(); }
};

extern std::vector<class power_meter *> power_meters;

extern void start_power_measurement(void);
extern void end_power_measurement(void);
extern double global_power(void);
extern void global_sample_power(void);
extern double global_joules(void);
extern double global_time_left(void);

extern void detect_power_meters(void);
extern void clear_power_meters(void);
extern void extech_power_meter(const std::string &devnode);

extern double min_power;

