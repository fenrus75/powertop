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

#include "device.h"
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

device::device(void)
{
	cached_valid = 0;
	hide = 0;
}

void device::register_sysfs_path(const std::string &path)
{
	std::string current_path = path;
	int iter = 0;
	char resolved_path[PATH_MAX + 1];

	while (iter++ < 10) {
		std::string test_path = current_path + "/device";
		if (access(test_path.c_str(), R_OK) == 0)
			current_path = test_path;
		else
			break;
	}

	if (realpath(current_path.c_str(), resolved_path))
		real_path = resolved_path;
	else
		real_path.clear();
}

void device::start_measurement(void)
{
	hide = false;
}

void device::end_measurement(void)
{
}

double device::utilization(void)
{
	return 0.0;
}

void device::collect_json_fields(std::string &_js)
{
	JSON_KV("class", class_name());
	JSON_KV("name", device_name());
	JSON_FIELD(hide);
	JSON_FIELD(guilty);
	JSON_FIELD(real_path);
}

std::vector<class device *> all_devices;
