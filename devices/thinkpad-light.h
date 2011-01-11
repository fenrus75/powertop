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
#ifndef _INCLUDE_GUARD_THINKPAD_LIGHT_H
#define _INCLUDE_GUARD_THINKPAD_LIGHT_H


#include "device.h"
#include "../parameters/parameters.h"

class thinkpad_light: public device {
	double start_rate, end_rate;
	int light_index;
	int r_index;
public:

	thinkpad_light();

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "light";};

	virtual const char * device_name(void) { return "Light-1";};
	virtual const char * human_name(void) { return "Thinkpad light";};
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual const char * util_units(void) { return "%"; };
	virtual int power_valid(void) { return utilization_power_valid(r_index);};
	virtual int grouping_prio(void) { return 1; };
};

extern void create_thinkpad_light(void);


#endif