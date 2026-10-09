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


#include "device.h"
#include "../parameters/parameters.h"

class thinkpad_light: public device {
	double start_rate = 0.0, end_rate = 0.0;
	int light_index = 0;
	int r_index = 0;
public:

	thinkpad_light();

	void start_measurement(void) override;
	void end_measurement(void) override;

	double	utilization(void) const override; /* percentage */

	std::string class_name(void) const override { return "light";};

	std::string device_name(void) const override { return "Light-1";};
	std::string human_name(void) override { return _("Thinkpad light");};
	double power_usage(struct result_bundle *result, struct parameter_bundle *bundle) override;
	std::string util_units(void) const override { return "%"; };
	bool power_valid(void) const override { return utilization_power_valid(r_index);};
	int grouping_prio(void) const override { return 1; };
	void collect_json_fields(std::string &_js) override;
};

extern void create_thinkpad_light(void);

