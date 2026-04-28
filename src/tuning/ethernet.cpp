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
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <utility>
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <errno.h>
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


int ethernet_tunable::good_bad(void)
{
	int sock;
	struct ifreq ifr;
	struct ethtool_wolinfo wol;
	int ret;
	int result = TUNE_GOOD;

	memset(&ifr, 0, sizeof(struct ifreq));

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock<0)
		return result;

	strncpy(ifr.ifr_name, interf.c_str(), IFNAMSIZ - 1);

	/* Check if the interf is up */
	ret = ioctl(sock, SIOCGIFFLAGS, &ifr);
	if (ret<0) {
		close(sock);
		return result;
	}

	memset(&wol, 0, sizeof(wol));

	wol.cmd = ETHTOOL_GWOL;
	ifr.ifr_data = (caddr_t)&wol;
	ret = ioctl(sock, SIOCETHTOOL, &ifr);
	if (ret < 0) {
		close(sock);
		return result;
	}

	if (wol.wolopts)
		result = TUNE_BAD;

	close(sock);

	return result;
}

void ethernet_tunable::toggle(void)
{
	int sock;
	struct ifreq ifr;
	struct ethtool_wolinfo wol;
	int ret;

	memset(&ifr, 0, sizeof(struct ifreq));

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock<0)
		return;

	strncpy(ifr.ifr_name, interf.c_str(), IFNAMSIZ - 1);

	/* Check if the interface is up */
	ret = ioctl(sock, SIOCGIFFLAGS, &ifr);
	if (ret<0) {
		close(sock);
		return;
	}

	memset(&wol, 0, sizeof(wol));

	wol.cmd = ETHTOOL_GWOL;
	ifr.ifr_data = (caddr_t)&wol;
	ret = ioctl(sock, SIOCETHTOOL, &ifr);
	if (ret < 0) {
		close(sock);
		return;
	}
	wol.cmd = ETHTOOL_SWOL;
	wol.wolopts = 0;
	ioctl(sock, SIOCETHTOOL, &ifr);

	close(sock);
}


void ethtunable_callback(const std::string &d_name)
{
	class ethernet_tunable *eth;
	if (d_name == "lo")
		return;
	eth = new(std::nothrow) class ethernet_tunable(d_name);
	if (eth)
		all_tunables.push_back(eth);
}

void add_ethernet_tunable(void)
{
	create_all_nics(&ethtunable_callback);
}
