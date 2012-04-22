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
#include "wifi.h"
#include <string.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "../lib.h"

extern "C" {
#include "iw.h"
}


wifi_tunable::wifi_tunable(const char *_iface) : tunable("", 1.5, _("Good"), _("Bad"), _("Unknown"))
{
	strcpy(iface, _iface);
	sprintf(desc, _("Wireless Power Saving for interface %s"), iface);

	sprintf(toggle_good, "iw dev %s set power_save off", iface);
	sprintf(toggle_bad, "iw dev %s set power_save on", iface);
}

int wifi_tunable::good_bad(void)
{
	if (get_wifi_power_saving(iface))
		return TUNE_GOOD;

	return TUNE_BAD;
}

void wifi_tunable::toggle(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		set_wifi_power_saving(iface, 0);
		return;
	}

	set_wifi_power_saving(iface, 1);
}

const char *wifi_tunable::toggle_script(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		return toggle_bad;
	}

	return toggle_good;
}

void add_wifi_tunables(void)
{
	struct dirent *entry;
	DIR *dir;
	char* wlan_name;
	class wifi_tunable *wifi;


	dir = opendir("/sys/class/net/");
	if (!dir)
		return;
	while (1) {
		entry = readdir(dir);
		if (!entry)
			break;
		wlan_name = strstr(entry->d_name, "wlan");
		if (wlan_name) {
			wifi = new class wifi_tunable(wlan_name);
			all_tunables.push_back(wifi);
		}
	
	}

	closedir(dir);

}
