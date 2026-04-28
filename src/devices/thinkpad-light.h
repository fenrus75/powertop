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

	virtual void start_measurement(void) override;
	virtual void end_measurement(void) override;

	virtual double	utilization(void) override; /* percentage */

	virtual std::string class_name(void) override { return "light";};

	virtual std::string device_name(void) override { return "Light-1";};
	virtual std::string human_name(void) override { return _("Thinkpad light");};
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle) override;
	virtual std::string util_units(void) override { return "%"; };
	virtual bool power_valid(void) override { return utilization_power_valid(r_index);};
	virtual int grouping_prio(void) override { return 1; };
};

extern void create_thinkpad_light(void);

