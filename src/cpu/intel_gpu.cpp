/*
 * Copyright 2012, Intel Corporation
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
#include "intel_cpus.h"
#include <iostream>
#include <fstream>
#include <format>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "../lib.h"
#include "../parameters/parameters.h"
#include "../display.h"

void i965_core::measurement_start(void)
{
	before = pt_gettime();
	rc6_before = read_sysfs("/sys/class/drm/card0/power/rc6_residency_ms", nullptr);
	rc6p_before = read_sysfs("/sys/class/drm/card0/power/rc6p_residency_ms", nullptr);
	rc6pp_before = read_sysfs("/sys/class/drm/card0/power/rc6pp_residency_ms", nullptr);

	update_cstate("gpu c0", "Powered On", 0, 0, 1, 0);
	update_cstate("gpu rc6", "RC6", 0, rc6_before, 1, 1);
	update_cstate("gpu rc6p", "RC6p", 0, rc6p_before, 1, 2);
	update_cstate("gpu rc6pp", "RC6pp", 0, rc6pp_before, 1, 3);
}

std::string i965_core::fill_cstate_line(int line_nr, const std::string &separator __unused)
{
	double ratio, d = -1.0, time_delta;

	if (line_nr == LEVEL_HEADER) {
		return _("  GPU ");
	}

	time_delta  = 1000000 * (after.tv_sec - before.tv_sec) + after.tv_usec - before.tv_usec;
	ratio = 100000.0/time_delta;

	switch (line_nr) {	
	case 0:
		d = 100.0 - ratio * (rc6_after + rc6p_after + rc6pp_after - rc6_before - rc6p_before - rc6pp_before);
		break;
	case 1:
		d = ratio * (rc6_after - rc6_before);
		break;
	case 2:
		d = ratio * (rc6p_after - rc6p_before);
		break;
	case 3:
		d = ratio * (rc6pp_after - rc6pp_before);
		break;
	default:
		return "";
	}
		
	/* cope with rounding errors due to the measurement interval */
	if (d < 0.0)
		d = 0.0;
	if (d > 100.0)
		d = 100.0;
	
	return std::format("{:5.1f}%", d);
}


void i965_core::measurement_end(void)
{
	after = pt_gettime();

	rc6_after = read_sysfs("/sys/class/drm/card0/power/rc6_residency_ms", nullptr);
	rc6p_after = read_sysfs("/sys/class/drm/card0/power/rc6p_residency_ms", nullptr);
	rc6pp_after = read_sysfs("/sys/class/drm/card0/power/rc6pp_residency_ms", nullptr);
}

std::string i965_core::fill_pstate_line(int line_nr __unused)
{
	return "";
}

std::string i965_core::fill_pstate_name(int line_nr __unused)
{
	return "";
}

