/*
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
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <cerrno>
#include <linux/types.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <format>

#include <linux/ethtool.h>

#include "../lib.h"
#include "wakeup_usb.h"

usb_wakeup::usb_wakeup([[maybe_unused]] const std::string &path, const std::string &iface) : wakeup("", 0.5, _("Enabled"), _("Disabled"))
{
	interf = iface;
	desc = pt_format(_("Wake status for USB device {}"), iface);
	usb_path = std::format("/sys/bus/usb/devices/{}/power/wakeup", iface);
	toggle_enable = std::format("echo 'enabled' > '{}';", usb_path);
	toggle_disable = std::format("echo 'disabled' > '{}';", usb_path);
}

int usb_wakeup::wakeup_value(void) const
{
	const std::string content = read_sysfs_string(usb_path);

	if (content == "enabled")
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

std::string usb_wakeup::wakeup_toggle_script(void) const
{
	const int enable = wakeup_value();

	if (enable == WAKEUP_ENABLE) {
		return toggle_disable;
	}

	return toggle_enable;
}

static void wakeup_usb_callback(const std::string &d_name)
{
	std::string filename;

	filename = std::format("/sys/bus/usb/devices/{}/power/wakeup", d_name);
	if (pt_access(filename, R_OK) != 0)
		return;

	filename = std::format("/sys/bus/usb/devices/{}/power/wakeup", d_name);
	wakeup_all.push_back(std::make_unique<usb_wakeup>(filename, d_name));
}

void add_usb_wakeup(void)
{
	process_directory("/sys/bus/usb/devices/", wakeup_usb_callback);
}

void usb_wakeup::collect_json_fields(std::string &_js) const
{
    wakeup::collect_json_fields(_js);
    JSON_FIELD(usb_path);
    JSON_FIELD(interf);
}
