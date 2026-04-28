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
#include "unistd.h"
#include "tuningusb.h"
#include <dirent.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <limits.h>
#include <format>

#include "../lib.h"

usb_tunable::usb_tunable(const std::string &path, const std::string &name) : tunable("", 0.9, _("Good"), _("Bad"), _("Unknown"))
{
	std::string vendor;
	std::string product;
	std::string str1, str2;

	usb_path = std::format("{}/power/control", path);

	str1 = read_sysfs_string(std::format("{}/idVendor", path));
	str2 = read_sysfs_string(std::format("{}/idProduct", path));

	desc = pt_format(_("Autosuspend for unknown USB device {} ({}:{})"), name, str1, str2);

	vendor = read_sysfs_string(std::format("{}/manufacturer", path));
	if (vendor.starts_with("Linux "))
		vendor = "";

	product = read_sysfs_string(std::format("{}/product", path));

	if (!vendor.empty() && !product.empty()) {
		desc = pt_format(_("Autosuspend for USB device {} [{}]"), product, vendor);
	} else if (!product.empty()) {
		desc = pt_format(_("Autosuspend for USB device {} [{}]"), product, name);
	} else if (!vendor.empty()) {
		desc = pt_format(_("Autosuspend for USB device {} [{}]"), vendor, name);
	}

	toggle_good = std::format("echo 'auto' > '{}';", usb_path);
	toggle_bad = std::format("echo 'on' > '{}';", usb_path);
}

int usb_tunable::good_bad(void)
{
	std::string content;

	content = read_sysfs_string(usb_path);

	if (content == "auto")
		return TUNE_GOOD;

	return TUNE_BAD;
}

void usb_tunable::toggle(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		write_sysfs(usb_path, "on");
		return;
	}

	write_sysfs(usb_path, "auto");
}

static void add_usb_callback(const std::string &d_name)
{
	class usb_tunable *usb;
	std::string filename;
	DIR *dir;

	filename = std::format("/sys/bus/usb/devices/{}/power/control", d_name);
	if (access(filename.c_str(), R_OK) != 0)
		return;

	filename = std::format("/sys/bus/usb/devices/{}/power/active_duration", d_name);
	if (access(filename.c_str(), R_OK)!=0)
		return;

	/* every interface of this device should support autosuspend */
	filename = std::format("/sys/bus/usb/devices/{}", d_name);
	if ((dir = opendir(filename.c_str()))) {
		struct dirent *entry;
		bool has_non_autosuspend = false;
		while ((entry = readdir(dir))) {
			/* dirname: <busnum>-<devnum>...:<config num>-<interface num> */
			if (!isdigit(entry->d_name[0]))
				continue;
			filename = std::format("/sys/bus/usb/devices/{}/{}/supports_autosuspend", d_name, entry->d_name);
			if (access(filename.c_str(), R_OK) == 0 && read_sysfs(filename) == 0) {
				has_non_autosuspend = true;
				break;
			}
		}
		closedir(dir);
		if (has_non_autosuspend)
			return;
	}

	filename = std::format("/sys/bus/usb/devices/{}", d_name);
	usb = new usb_tunable(filename, d_name);
	all_tunables.push_back(usb);
}

void add_usb_tunables(void)
{
	process_directory("/sys/bus/usb/devices/", add_usb_callback);
}
