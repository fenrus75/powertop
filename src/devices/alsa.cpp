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

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "device.h"
#include "alsa.h"
#include "../parameters/parameters.h"

#include "../devlist.h"

#include <unistd.h>
#include <format>

alsa::alsa(const std::string &_name, const std::string &path): device()
{
	std::string model;
	std::string vendor;
	end_active = 0;
	start_active = 0;
	end_inactive = 0;
	start_inactive = 0;
	sysfs_path = path;

	name = std::format("alsa:{}", _name);
	humanname = std::format("alsa:{}", _name);
	rindex = get_result_index(name);

	model = read_sysfs_string(std::format("{}/modelname", path));
	vendor = read_sysfs_string(std::format("{}/vendor_name", path));

	if (!model.empty() && !vendor.empty())
		humanname = pt_format(_("Audio codec {}: {} ({})"), name, model, vendor);
	else if (!model.empty())
		humanname = pt_format(_("Audio codec {}: {}"), _name, model);
	else if (!vendor.empty())
		humanname = pt_format(_("Audio codec {}: {}"), _name, vendor);
}

void alsa::start_measurement(void)
{
	std::string content;

	content = read_sysfs_string(std::format("{}/power_off_acct", sysfs_path));
	if (!content.empty()) {
		try {
			start_inactive = std::stoull(content);
		} catch (...) {}
	}

	content = read_sysfs_string(std::format("{}/power_on_acct", sysfs_path));
	if (!content.empty()) {
		try {
			start_active = std::stoull(content);
		} catch (...) {}
	}
}

void alsa::end_measurement(void)
{
	std::string content;
	double p;

	content = read_sysfs_string(std::format("{}/power_off_acct", sysfs_path));
	if (!content.empty()) {
		try {
			end_inactive = std::stoull(content);
		} catch (...) {}
	}

	content = read_sysfs_string(std::format("{}/power_on_acct", sysfs_path));
	if (!content.empty()) {
		try {
			end_active = std::stoull(content);
		} catch (...) {}
	}

	double active_delta = (end_active >= start_active) ? (double)(end_active - start_active) : 0.0;
	double total_delta = (end_active + end_inactive >= start_active + start_inactive) ?
		(double)(end_active + end_inactive - start_active - start_inactive) : 0.0;
	p = active_delta / (0.001 + total_delta) * 100.0;
	report_utilization(name, p);
}


double alsa::utilization(void)
{
	double p;

	double active_delta = (end_active >= start_active) ? (double)(end_active - start_active) : 0.0;
	double total_delta = (end_active + end_inactive >= start_active + start_inactive) ?
		(double)(end_active + end_inactive - start_active - start_inactive) : 0.0;
	p = active_delta / (0.001 + total_delta) * 100.0;

	return p;
}

static void create_all_alsa_callback(const std::string &d_name)
{
	class alsa *bl;

	if (!d_name.starts_with("hwC"))
		return;

	if (access(std::format("/sys/class/sound/{}/power_on_acct", d_name).c_str(), R_OK) != 0)
		return;

	bl = new class alsa(d_name, std::format("/sys/class/sound/{}", d_name));
	all_devices.push_back(bl);
	register_parameter("alsa-codec-power", 0.5);
}

void create_all_alsa(void)
{
	process_directory("/sys/class/sound/", create_all_alsa_callback);
}

double alsa::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;
	static int index = -1;

	power = 0;
	if (index < 0)
		index = get_param_index("alsa-codec-power");

	factor = get_parameter_value(index, bundle);

	util = get_result_value(rindex, result);

	power += util * factor / 100.0;

	return power;
}

void alsa::register_power_with_devlist(struct result_bundle *results, struct parameter_bundle *bundle)
{
	if (name.length() > 7)
		register_devpower(name.substr(7), power_usage(results, bundle), this);
	else
		register_devpower(name, power_usage(results, bundle), this);
}

std::string alsa::human_name(void)
{
	if (!guilty.empty())
		return std::format("{} ({})", humanname, guilty);

	return humanname;
}
