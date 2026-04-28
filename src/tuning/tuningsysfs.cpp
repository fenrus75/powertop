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

#include "tuning.h"
#include "tunable.h"
#include <unistd.h>
#include "tuningsysfs.h"
#include <dirent.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <limits.h>
#include <format>

#include "../lib.h"
sysfs_tunable::sysfs_tunable(const std::string &str, const std::string &_sysfs_path, const std::string &target_content) : tunable(str, 1.0, _("Good"), _("Bad"), _("Unknown"))
{
	sysfs_path = _sysfs_path;
	target_value = target_content;

	toggle_good = std::format("echo '{}' > '{}';", target_value, sysfs_path);
}

int sysfs_tunable::good_bad(void)
{
	std::string content;

	content = read_sysfs_string(sysfs_path);

	if (content == target_value)
		return TUNE_GOOD;

	bad_value = content;
	toggle_bad = std::format("echo '{}' > '{}';", bad_value, sysfs_path);
	return TUNE_BAD;
}

void sysfs_tunable::toggle(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		if (!bad_value.empty())
			write_sysfs(sysfs_path, bad_value);
		return;
	}

	write_sysfs(sysfs_path, target_value);
}


void add_sysfs_tunable(const std::string &str, const std::string &_sysfs_path, const std::string &_target_content)
{
	if (access(_sysfs_path.c_str(), R_OK) != 0)
		return;
	class sysfs_tunable *st;

	st = new sysfs_tunable(str, _sysfs_path, _target_content);
	all_tunables.push_back(st);
}

static void add_sata_callback(const std::string &d_name)
{
	std::string filename;
	filename = std::format("/sys/class/scsi_host/{}/link_power_management_policy", d_name);
	if (access(filename.c_str(), R_OK) != 0)
		return;

	add_sysfs_tunable(pt_format(_("Enable SATA link power management for {}"), d_name), filename, "med_power_with_dipm");
}

void add_sata_tunables(void)
{
	process_directory("/sys/class/scsi_host/", add_sata_callback);
}
