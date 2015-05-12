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
#ifndef _INCLUDE_GUARD_AHCI_H
#define _INCLUDE_GUARD_AHCI_H


#include <string>
#include <limits.h>
#include "device.h"
#include "../parameters/parameters.h"
#include <stdint.h>

class ahci: public device {
	uint64_t start_active, end_active;
	uint64_t start_partial, end_partial;
	uint64_t start_slumber, end_slumber;
	uint64_t start_devslp, end_devslp;
	char sysfs_path[PATH_MAX];
	char name[4096];
	int partial_rindex;
	int active_rindex;
	int slumber_rindex;
	int devslp_rindex;
	int partial_index;
	int active_index;
	char humanname[4096];
public:

	ahci(char *_name, char *path);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "ahci";};

	virtual const char * device_name(void);
	virtual const char * human_name(void) { return humanname;};
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual int power_valid(void) { return utilization_power_valid(partial_rindex) + utilization_power_valid(active_rindex);};
	virtual int grouping_prio(void) { return 1; };
	virtual void report_device_stats(string *ahci_data, int idx);
};

extern void create_all_ahcis(void);
extern void ahci_create_device_stats_table(void);


#endif
