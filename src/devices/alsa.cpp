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

using namespace std;

#include "device.h"
#include "alsa.h"
#include "../parameters/parameters.h"

#include "../devlist.h"

#include <string.h>
#include <unistd.h>
#include <format>

alsa::alsa(const string &_name, const string &path): device()
{
	ifstream file;

	char model[4096];
	char vendor[4096];
	end_active = 0;
	start_active = 0;
	end_inactive = 0;
	start_inactive = 0;
	sysfs_path = path;

	name = std::format("alsa:{}", _name);
	humanname = std::format("alsa:{}", _name);
	rindex = get_result_index(name);

	model[0] = 0;
	vendor[0] = 0;
	file.open(std::format("{}/modelname", path));
	if (file) {
		file.getline(model, sizeof(model));
		file.close();
	}
	file.open(std::format("{}/vendor_name", path));
	if (file) {
		file.getline(vendor, sizeof(vendor));
		file.close();
	}
	if (strlen(model) && strlen(vendor))
		humanname = pt_format(_("Audio codec {}: {} ({})"), name, model, vendor);
	else if (strlen(model))
		humanname = pt_format(_("Audio codec {}: {}"), _name, model);
	else if (strlen(vendor))
		humanname = pt_format(_("Audio codec {}: {}"), _name, vendor);
}

void alsa::start_measurement(void)
{
	ifstream file;

	try {
		file.open(std::format("{}/power_off_acct", sysfs_path));
		if (file) {
			file >> start_inactive;
		}
		file.close();
		file.open(std::format("{}/power_on_acct", sysfs_path));

		if (file) {
			file >> start_active;
		}
		file.close();
	}
	catch (std::ios_base::failure &c) {
		fprintf(stderr, "%s\n", c.what());
	}
}

void alsa::end_measurement(void)
{
	ifstream file;
	double p;

	try {
		file.open(std::format("{}/power_off_acct", sysfs_path));
		if (file) {
			file >> end_inactive;
		}
		file.close();
		file.open(std::format("{}/power_on_acct", sysfs_path));

		if (file) {
			file >> end_active;
		}
		file.close();
	}
	catch (std::ios_base::failure &c) {
		fprintf(stderr, "%s\n", c.what());
	}

	p = (end_active - start_active) / (0.001 + end_active + end_inactive - start_active - start_inactive) * 100.0;
	report_utilization(name, p);
}


double alsa::utilization(void)
{
	double p;

	p = (end_active - start_active) / (0.001 + end_active - start_active + end_inactive - start_inactive) * 100.0;

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
	static int index = 0;

	power = 0;
	if (!index)
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
