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
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
#include "ethernet.h"

extern void create_all_nics(callback fn);

ethernet_tunable::ethernet_tunable(const std::string &iface) : tunable("", 0.3, _("Good"), _("Bad"), _("Unknown"))
{
	interf = iface;
	desc = pt_format(_("Wake-on-lan status for device {}"), iface);
	toggle_good = std::format("ethtool -s {} wol d;", iface);

}


int ethernet_tunable::get_wol(uint32_t &wolopts) const
{
	struct ifreq ifr;
	struct ethtool_wolinfo wol;

	memset(&ifr, 0, sizeof(struct ifreq));

	const int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return -1;

	strncpy(ifr.ifr_name, interf.c_str(), IFNAMSIZ - 1);

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
		close(sock);
		return -1;
	}

	memset(&wol, 0, sizeof(wol));
	wol.cmd = ETHTOOL_GWOL;
	ifr.ifr_data = (caddr_t)&wol;

	if (ioctl(sock, SIOCETHTOOL, &ifr) < 0) {
		close(sock);
		return -1;
	}

	close(sock);
	wolopts = wol.wolopts;
	return 0;
}

void ethernet_tunable::set_wol(uint32_t wolopts)
{
	struct ifreq ifr;
	struct ethtool_wolinfo wol;

	memset(&ifr, 0, sizeof(struct ifreq));
	memset(&wol, 0, sizeof(wol));

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return;

	strncpy(ifr.ifr_name, interf.c_str(), IFNAMSIZ - 1);
	wol.cmd = ETHTOOL_SWOL;
	wol.wolopts = wolopts;
	ifr.ifr_data = (caddr_t)&wol;

	ioctl(sock, SIOCETHTOOL, &ifr);
	close(sock);
}

int ethernet_tunable::good_bad(void)
{
	uint32_t wolopts;

	if (get_wol(wolopts) < 0)
		return TUNE_GOOD;

	return wolopts ? TUNE_BAD : TUNE_GOOD;
}

void ethernet_tunable::toggle(void)
{
	uint32_t wolopts;

	if (get_wol(wolopts) < 0)
		return;

	set_wol(0);
}


void ethtunable_callback(const std::string &d_name)
{
	if (d_name == "lo")
		return;
	all_tunables.push_back(std::make_unique<ethernet_tunable>(d_name));
}

void add_ethernet_tunable(void)
{
	create_all_nics(&ethtunable_callback);
}

void ethernet_tunable::collect_json_fields(std::string &_js)
{
    tunable::collect_json_fields(_js);
    JSON_FIELD(interf);
}
