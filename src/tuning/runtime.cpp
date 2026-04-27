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
#include <limits.h>
#include <format>

#include "../lib.h"
#include "../devices/runtime_pm.h"

runtime_tunable::runtime_tunable(const string &path, const string &bus, const string &dev, const string &port) : tunable("", 0.4, _("Good"), _("Bad"), _("Unknown"))
{
	runtime_path = std::format("{}/power/control", path);


	desc = pt_format(_("Runtime PM for {} device {}"), bus, dev);
	if (!device_has_runtime_pm(path)) {
		desc = pt_format(_("{} device {} has no runtime power management"), bus, dev);
	}

	if (bus == "pci") {
		std::string filename;
		uint16_t vendor = 0, device = 0;
		std::string content;

		content = read_sysfs_string(std::format("/sys/bus/{}/devices/{}/vendor", bus, dev));
		if (!content.empty()) {
			try {
				vendor = std::stoul(content, nullptr, 16);
			} catch (...) {}
		}


		content = read_sysfs_string(std::format("/sys/bus/{}/devices/{}/device", bus, dev));
		if (!content.empty()) {
			try {
				device = std::stoul(content, nullptr, 16);
			} catch (...) {}
		}

		if (vendor && device) {
			if (!device_has_runtime_pm(path)) {
				desc = pt_format(_("PCI Device {} has no runtime power management"), pci_id_to_name(vendor, device));
			} else {
				desc = pt_format(_("Runtime PM for PCI Device {}"), pci_id_to_name(vendor, device));
			}
		}

		if (path.find("ata") != string::npos) {
			desc = pt_format(_("Runtime PM for port {} of PCI device: {}"), port, pci_id_to_name(vendor, device));
		}

		if (path.find("block") != string::npos) {
			desc = pt_format(_("Runtime PM for disk {}"), port);
		}

	}
	toggle_good = std::format("echo 'auto' > '{}';", runtime_path);
	toggle_bad = std::format("echo 'on' > '{}';", runtime_path);
}

int runtime_tunable::good_bad(void)
{
	std::string content;

	content = read_sysfs_string(runtime_path);

	if (content == "auto")
		return TUNE_GOOD;
	if (content == "on")
		return TUNE_GOOD;

	return TUNE_BAD;
}

void runtime_tunable::toggle(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		write_sysfs(runtime_path.c_str(), "on");
		return;
	}

	write_sysfs(runtime_path.c_str(), "auto");
}


void add_runtime_tunables(const std::string &bus)
{
	struct dirent *entry;
	DIR *dir;
	std::string filename, port;
	int max_ports = 32, count=0;

	filename = std::format("/sys/bus/{}/devices/", bus);
	dir = opendir(filename.c_str());
	if (!dir)
		return;
	while (1) {
		class runtime_tunable *runtime, *runtime_ahci_port, *runtime_ahci_disk;

		entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		filename = std::format("/sys/bus/{}/devices/{}/power/control", bus, entry->d_name);

		if (access(filename.c_str(), R_OK) != 0)
			continue;


		filename = std::format("/sys/bus/{}/devices/{}", bus, entry->d_name);

		runtime = new class runtime_tunable(filename, bus, entry->d_name, "");

		if (!device_has_runtime_pm(filename))
			all_untunables.push_back(runtime);
		else
			all_tunables.push_back(runtime);

		for (int i=0; i < max_ports; i++) {
			port = std::format("ata{}", i);
			filename = std::format("/sys/bus/{}/devices/{}/{}/power/control", bus, entry->d_name, port);

			if (access(filename.c_str(), R_OK) != 0)
				continue;

			filename = std::format("/sys/bus/{}/devices/{}/{}", bus, entry->d_name, port);
			runtime_ahci_port = new class runtime_tunable(filename, bus, entry->d_name, port);

			if (!device_has_runtime_pm(filename))
				all_untunables.push_back(runtime_ahci_port);
			else
				all_tunables.push_back(runtime_ahci_port);
		}

		for (char blk = 'a'; blk <= 'z'; blk++)
		{
			if (count != 0)
				break;

			filename = std::format("/sys/block/sd{}/device/power/control", blk);

			if (access(filename.c_str(), R_OK) != 0)
				continue;

			port = std::format("sd{}", blk);
			filename = std::format("/sys/block/{}/device", port);
			runtime_ahci_disk = new class runtime_tunable(filename, bus, entry->d_name, port);
			if (!device_has_runtime_pm(filename))
				all_untunables.push_back(runtime_ahci_disk);
			else
				all_tunables.push_back(runtime_ahci_disk);
		}
		count = 1;

	}
	closedir(dir);
}
