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
#ifndef _INCLUDE_GUARD_ALSA_H
#define _INCLUDE_GUARD_ALSA_H


#include "device.h"
#include "../parameters/parameters.h"

#include <stdint.h>
#include <limits.h>

class alsa: public device {
	uint64_t start_active, end_active;
	uint64_t start_inactive, end_inactive;
	char sysfs_path[PATH_MAX];
	char name[4096];
	char humanname[4096];
	char temp_buf[4096];
	int rindex;
public:

	alsa(const char *_name, const char *path);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "alsa";};

	virtual const char * device_name(void);
	virtual const char * human_name(void);
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual int power_valid(void) { return utilization_power_valid(rindex);};

	virtual void register_power_with_devlist(struct result_bundle *results, struct parameter_bundle *bundle);
	virtual int grouping_prio(void) { return 0; };

};

extern void create_all_alsa(void);


#endif
