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
#include "runtime.h"
#include <string.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "../lib.h"
#include "../devices/runtime_pm.h"

runtime_tunable::runtime_tunable(const char *path, const char *bus, const char *dev) : tunable("", 0.4, _("Good"), _("Bad"), _("Unknown"))
{
	ifstream file;
	sprintf(runtime_path, "%s/power/control", path);


	sprintf(desc, _("Runtime PM for %s device %s"), bus, dev);
	if (!device_has_runtime_pm(path))
		sprintf(desc, _("%s device %s has no runtime power management"), bus, dev);

	if (strcmp(bus, "pci") == 0) {
		char filename[4096];
		uint16_t vendor = 0, device = 0;

		sprintf(filename, "/sys/bus/%s/devices/%s/vendor", bus, dev);

		file.open(filename, ios::in);
		if (file) {
			file >> hex >> vendor;
			file.close();
		}


		sprintf(filename, "/sys/bus/%s/devices/%s/device", bus, dev);
		file.open(filename, ios::in);
		if (file) {
			file >> hex >> device;
			file.close();
		}

		if (vendor && device) {
			if (!device_has_runtime_pm(path))
				sprintf(desc, _("PCI Device %s has no runtime power management"), pci_id_to_name(vendor, device, filename, 4095));
			else
				sprintf(desc, _("Runtime PM for PCI Device %s"), pci_id_to_name(vendor, device, filename, 4095));
		}


	}
	sprintf(toggle_good, "echo 'auto' > '%s';", runtime_path);
	sprintf(toggle_bad, "echo 'on' > '%s';", runtime_path);
}

int runtime_tunable::good_bad(void)
{
	string content;

	content = read_sysfs_string(runtime_path);

	if (strcmp(content.c_str(), "auto") == 0)
		return TUNE_GOOD;

	return TUNE_BAD;
}

void runtime_tunable::toggle(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		write_sysfs(runtime_path, "on");
		return;
	}

	write_sysfs(runtime_path, "auto");
}

const char *runtime_tunable::toggle_script(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		return toggle_bad;
	}

	return toggle_good;
}


void add_runtime_tunables(const char *bus)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];

	sprintf(filename, "/sys/bus/%s/devices/", bus);
	dir = opendir(filename);
	if (!dir)
		return;
	while (1) {
		class runtime_tunable *runtime;

		entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		sprintf(filename, "/sys/bus/%s/devices/%s/power/control", bus, entry->d_name);

		if (access(filename, R_OK) != 0)
			continue;


		sprintf(filename, "/sys/bus/%s/devices/%s", bus, entry->d_name);

		runtime = new class runtime_tunable(filename, bus, entry->d_name);

		if (!device_has_runtime_pm(filename))
			all_untunables.push_back(runtime);
		else
			all_tunables.push_back(runtime);

	}
	closedir(dir);
}
