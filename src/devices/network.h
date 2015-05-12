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
#ifndef _INCLUDE_GUARD_NETWORK_H
#define _INCLUDE_GUARD_NETWORK_H

#include <sys/time.h>
#include <limits.h>

#include "device.h"
#include "../parameters/parameters.h"

class network: public device {
	int start_up, end_up;
	uint64_t start_pkts, end_pkts;
	struct timeval before, after;

	int start_speed; /* 0 is "no link" */
	int end_speed; /* 0 is "no link" */

	char sysfs_path[PATH_MAX];
	char name[4096];
	char humanname[4096];
	int index_up;
	int rindex_up;
	int index_link_100;
	int rindex_link_100;
	int index_link_1000;
	int rindex_link_1000;
	int index_link_high;
	int rindex_link_high;
	int index_pkts;
	int rindex_pkts;
	int index_powerunsave;
	int rindex_powerunsave;

	int valid_100;
	int valid_1000;
	int valid_high;
	int valid_powerunsave;
public:
	uint64_t pkts;
	double duration;

	network(const char *_name, const char *path);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void);
	virtual const char * util_units(void) { return " pkts/s"; };

	virtual const char * class_name(void) { return "ethernet";};

	virtual const char * device_name(void);
	virtual const char * human_name(void) { return humanname; };
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual int power_valid(void) { return utilization_power_valid(rindex_up) + utilization_power_valid(rindex_link_100) + utilization_power_valid(rindex_link_1000)  + utilization_power_valid(rindex_link_high);};
	virtual int grouping_prio(void) { return 10; };
};

extern void create_all_nics(callback fn = NULL);

#endif
