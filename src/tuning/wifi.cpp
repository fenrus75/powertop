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
#include "wifi.h"
#include <utility>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <format>

#include "../lib.h"

extern "C" {
#include "iw.h"
}


wifi_tunable::wifi_tunable(const std::string &_iface) : tunable("", 1.5, _("Good"), _("Bad"), _("Unknown"))
{
	iface = _iface;
	desc = pt_format(_("Wireless Power Saving for interface {}"), iface);

	toggle_good = std::format("iw dev {} set power_save on", iface);
	toggle_bad = std::format("iw dev {} set power_save off", iface);
}

int wifi_tunable::good_bad(void)
{
	if (get_wifi_power_saving(iface.c_str()))
		return TUNE_GOOD;

	return TUNE_BAD;
}

void wifi_tunable::toggle(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		set_wifi_power_saving(iface.c_str(), 0);
		return;
	}

	set_wifi_power_saving(iface.c_str(), 1);
}

void add_wifi_tunables(void)
{
	struct dirent *entry;
	DIR *dir;
	class wifi_tunable *wifi;


	dir = opendir("/sys/class/net/");
	if (!dir)
		return;
	while (true) {
		entry = readdir(dir);
		if (!entry)
			break;
		std::string name(entry->d_name);
		if (name.starts_with("wlan") || name.starts_with("wlp") || name.starts_with("wlx")) {
			wifi = new(std::nothrow) class wifi_tunable(name);
			if (wifi)
				all_tunables.push_back(wifi);
		}
	}

	closedir(dir);

}
