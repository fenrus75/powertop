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
#include <string.h>
#include <dirent.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <limits.h>

#include "../lib.h"

usb_tunable::usb_tunable(const char *path, const char *name) : tunable("", 0.9, _("Good"), _("Bad"), _("Unknown"))
{
	ifstream file;
	char filename[PATH_MAX];
	char vendor[2048];
	char product[2048];
	string str1, str2;
	snprintf(usb_path, PATH_MAX, "%s/power/control", path);

	vendor[0] = 0;
	product[0] = 0;

	str1 = read_sysfs_string("%s/idVendor", path);
	str2 = read_sysfs_string("%s/idProduct", path);

	snprintf(desc, 4096, _("Autosuspend for unknown USB device %s (%s:%s)"), name, str1.c_str(), str2.c_str());

	snprintf(filename, PATH_MAX, "%s/manufacturer", path);
	file.open(filename, ios::in);
	if (file) {
		file.getline(vendor, 2047);
		if (strstr(vendor, "Linux "))
			vendor[0] = 0;
		file.close();
	};
	snprintf(filename, PATH_MAX, "%s/product", path);
	file.open(filename, ios::in);
	if (file) {
		file.getline(product, 2040);
		file.close();
	};
	if (strlen(vendor) && strlen(product))
		snprintf(desc, 4096, _("Autosuspend for USB device %s [%s]"), product, vendor);
	else if (strlen(product))
		snprintf(desc, 4096, _("Autosuspend for USB device %s [%s]"), product, name);
	else if (strlen(vendor))
		snprintf(desc, 4096, _("Autosuspend for USB device %s [%s]"), vendor, name);

	snprintf(toggle_good, 4096, "echo 'auto' > '%s';", usb_path);
	snprintf(toggle_bad, 4096, "echo 'on' > '%s';", usb_path);
}

int usb_tunable::good_bad(void)
{
	string content;

	content = read_sysfs_string(usb_path);

	if (strcmp(content.c_str(), "auto") == 0)
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

const char *usb_tunable::toggle_script(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		return toggle_bad;
	}

	return toggle_good;
}

static void add_usb_callback(const char *d_name)
{
	class usb_tunable *usb;
	char filename[PATH_MAX];
	DIR *dir;

	snprintf(filename, PATH_MAX, "/sys/bus/usb/devices/%s/power/control", d_name);
	if (access(filename, R_OK) != 0)
		return;

	snprintf(filename, PATH_MAX, "/sys/bus/usb/devices/%s/power/active_duration", d_name);
	if (access(filename, R_OK)!=0)
		return;

	/* every interface of this device should support autosuspend */
	snprintf(filename, PATH_MAX, "/sys/bus/usb/devices/%s", d_name);
	if ((dir = opendir(filename))) {
		struct dirent *entry;
		while ((entry = readdir(dir))) {
			/* dirname: <busnum>-<devnum>...:<config num>-<interface num> */
			if (!isdigit(entry->d_name[0]))
				continue;
			snprintf(filename, PATH_MAX, "/sys/bus/usb/devices/%s/%s/supports_autosuspend", d_name, entry->d_name);
			if (access(filename, R_OK) == 0 && read_sysfs(filename) == 0)
				break;
		}
		closedir(dir);
		if (entry)
			return;
	}

	snprintf(filename, PATH_MAX, "/sys/bus/usb/devices/%s", d_name);
	usb = new class usb_tunable(filename, d_name);
	all_tunables.push_back(usb);
}

void add_usb_tunables(void)
{
	process_directory("/sys/bus/usb/devices/", add_usb_callback);
}
