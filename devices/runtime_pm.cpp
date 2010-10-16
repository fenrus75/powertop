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
#include "runtime_pm.h"

#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "../parameters/parameters.h"

#include <iostream>
#include <fstream>

runtime_pmdevice::runtime_pmdevice(const char *_name, const char *path, const char *devid)
{
}



void runtime_pmdevice::start_measurement(void)
{
}

void runtime_pmdevice::end_measurement(void)
{
}

double runtime_pmdevice::utilization(void) /* percentage */
{
	return 0.0;
}

const char * runtime_pmdevice::device_name(void)
{
	return name;
}

const char * runtime_pmdevice::human_name(void)
{
	return humanname;
}


double runtime_pmdevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	return 0.0;
}


void create_all_runtime_pm_devices(void)
{
}

