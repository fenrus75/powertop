;/*
 * Copyright 2018, Intel Corporation
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
 *      Gayatri Kammela <gayatri.kammela@intel.com>
 */

#include "wakeup.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <errno.h>
#include <linux/types.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>

#include <linux/ethtool.h>

#include "../lib.h"
#include "wakeup_usb.h"

usb_wakeup::usb_wakeup(const char *path, const char *iface) : wakeup("", 0.5, _("Enabled"), _("Disabled"))
{
	memset(interf, 0, sizeof(interf));
	pt_strcpy(interf, iface);
	sprintf(desc, _("Wake status for USB device %s"), iface);
	snprintf(usb_path, sizeof(usb_path), "/sys/bus/usb/devices/%s/power/wakeup", iface);
	snprintf(toggle_enable, sizeof(toggle_enable), "echo 'enabled' > '%s';", usb_path);
	snprintf(toggle_disable, sizeof(toggle_disable), "echo 'disabled' > '%s';", usb_path);
}

int usb_wakeup::wakeup_value(void)
{
	string content;

	content = read_sysfs_string(usb_path);

	if (strcmp(content.c_str(), "enabled") == 0)
		return WAKEUP_ENABLE;

	return WAKEUP_DISABLE;
}

void usb_wakeup::wakeup_toggle(void)
{
	int enable;
	enable = wakeup_value();

	if (enable == WAKEUP_ENABLE) {
		write_sysfs(usb_path, "disabled");
		return;
	}

	write_sysfs(usb_path, "enabled");
}

const char *usb_wakeup::wakeup_toggle_script(void)
{
	int enable;
	enable = wakeup_value();

	if (enable == WAKEUP_ENABLE) {
		return toggle_disable;
	}

	return toggle_enable;
}

void wakeup_usb_callback(const char *d_name)
{
	class usb_wakeup *usb;
	char filename[PATH_MAX];

	snprintf(filename, sizeof(filename), "/sys/bus/usb/devices/%s/power/wakeup", d_name);
	if (access(filename, R_OK) != 0)
		return;

	snprintf(filename, sizeof(filename), "/sys/bus/usb/devices/%s/power/wakeup", d_name);
	usb = new class usb_wakeup(filename, d_name);
	wakeup_all.push_back(usb);
}

void add_usb_wakeup(void)
{
	process_directory("/sys/bus/usb/devices/", wakeup_usb_callback);
}
