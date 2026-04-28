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
#include <iostream>
#include <fstream>
#include <filesystem>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>


#include "device.h"
#include "rfkill.h"
#include "../parameters/parameters.h"

#include <unistd.h>
#include <format>

rfkill::rfkill(const std::string &_name, const std::string &path): device()
{
	std::error_code ec;
	auto link = std::filesystem::read_symlink(std::format("{}/device/driver", path), ec);
	if (!ec)
		humanname = pt_format(_("Radio device: {}"), link.filename().string());
	link = std::filesystem::read_symlink(std::format("{}/device/device/driver", path), ec);
	if (!ec)
		humanname = pt_format(_("Radio device: {}"), link.filename().string());
}

void rfkill::start_measurement(void)
{
	start_hard = read_sysfs(std::format("{}/hard", sysfs_path));
	start_soft = read_sysfs(std::format("{}/soft", sysfs_path));

	end_hard = 1;
	end_soft = 1;
}

void rfkill::end_measurement(void)
{
	end_hard = read_sysfs(std::format("{}/hard", sysfs_path));
	end_soft = read_sysfs(std::format("{}/soft", sysfs_path));

	report_utilization(name, utilization());
}


double rfkill::utilization(void)
{
	double p;
	int rfk;

	rfk = start_soft+end_soft;
	if (rfk <  start_hard+end_hard)
		rfk = start_hard+end_hard;

	p = 100 - 50.0 * rfk;

	return p;
}

static void create_all_rfkills_callback(const std::string &d_name)
{
	std::string name = read_sysfs_string(std::format("/sys/class/rfkill/{}/name", d_name));

	if (name.empty())
		name = d_name;

	class rfkill *bl = new rfkill(name, std::format("/sys/class/rfkill/{}", d_name));
	all_devices.push_back(bl);
}

void create_all_rfkills(void)
{
	process_directory("/sys/class/rfkill/", create_all_rfkills_callback);
}

double rfkill::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;

	power = 0;
	factor = get_parameter_value(index, bundle);
	util = get_result_value(rindex, result);

	power += util * factor / 100.0;

	return power;
}
