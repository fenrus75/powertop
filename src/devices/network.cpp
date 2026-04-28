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
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>

#include <stdio.h>
#include <sys/types.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/ethtool.h>

#include "device.h"
#include "network.h"
#include "../lib.h"
#include "../parameters/parameters.h"
#include "../process/process.h"
#include "../tuning/wifi.h"
extern "C" {
#include "../tuning/iw.h"
}

#include <cstring>
#include <format>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sstream>

static std::map<std::string, class network *> nics;

static void do_proc_net_dev(void)
{
	static time_t last_time;
	class network *dev;

	if (time(nullptr) == last_time)
		return;

	last_time = time(nullptr);

	std::string content = read_file_content("/proc/net/dev");
	if (content.empty())
		return;

	std::istringstream stream(content);
	std::string line;

	/* Skip first two header lines */
	if (!std::getline(stream, line)) return;
	if (!std::getline(stream, line)) return;

	while (std::getline(stream, line)) {
		int i = 0;
		uint64_t pkt = 0;

		size_t pos = line.find(':');
		if (pos == std::string::npos)
			continue;

		std::string name = line.substr(0, pos);
		/* Trim leading spaces */
		name.erase(0, name.find_first_not_of(' '));

		dev = nics[name];
		if (!dev)
			continue;

		std::istringstream iss(line.substr(pos + 1));
		std::string val_s;
		while (iss >> val_s) {
			unsigned long long val = 0;
			try {
				val = std::stoull(val_s);
			} catch (...) {}
			i++;
			if (i == 2 || i == 10)
				pkt += val;

		}
		dev->pkts = pkt;
	}
}

#ifdef DISABLE_TRYCATCH

static inline void ethtool_cmd_speed_set(struct ethtool_cmd *ep,
						__u32 speed)
{

	ep->speed = (__u16)speed;
	ep->speed_hi = (__u16)(speed >> 16);
}

static inline __u32 ethtool_cmd_speed(struct ethtool_cmd *ep)
{
	return (ep->speed_hi << 16) | ep->speed;
}

#endif


network::network(const std::string &_name, const std::string &path): device()
{
	start_up = 0;
	end_up = 0;
	start_speed = 0;
	end_speed = 0;
	start_pkts = 0;
	end_pkts = 0;
	pkts = 0;
	valid_100 = -1;
	valid_1000 = -1;
	valid_high = -1;
	valid_powerunsave = -1;

	sysfs_path = path;
	register_sysfs_path(sysfs_path);
	name = _name;
	humanname = std::format("nic:{}", _name);

	index_up = get_param_index(std::format("{}-up", _name));
	rindex_up = get_result_index(std::format("{}-up", _name));

	index_powerunsave = get_param_index(std::format("{}-powerunsave", _name));
	rindex_powerunsave = get_result_index(std::format("{}-powerunsave", _name));

	index_link_100 = get_param_index(std::format("{}-link-100", _name));
	rindex_link_100 = get_result_index(std::format("{}-link-100", _name));

	index_link_1000 = get_param_index(std::format("{}-link-1000", _name));
	rindex_link_1000 = get_result_index(std::format("{}-link-1000", _name));

	index_link_high = get_param_index(std::format("{}-link-high", _name));
	rindex_link_high = get_result_index(std::format("{}-link-high", _name));

	index_pkts = get_param_index(std::format("{}-packets", _name));
	rindex_pkts = get_result_index(std::format("{}-packets", _name));

	char buf[4096];
	ssize_t len = readlink(std::format("{}/device/driver", path).c_str(), buf, sizeof(buf) - 1);
	if (len != -1) {
		buf[len] = '\0';
		humanname = pt_format(_("Network interface: {} ({})"), _name, basename(buf));
	};
}

static int net_iface_up(const std::string &iface)
{
	int sock;
	struct ifreq ifr;
	int ret;

	memset(&ifr, 0, sizeof(struct ifreq));

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock<0)
		return 0;

	pt_strcpy(ifr.ifr_name, iface.c_str());

	/* Check if the interface is up */
	ret = ioctl(sock, SIOCGIFFLAGS, &ifr);
	if (ret<0) {
		close(sock);
		return 0;
	}

	if (ifr.ifr_flags & (IFF_UP | IFF_RUNNING)) {
		close(sock);
		return 1;
	}

	close(sock);

	return 0;
}

static int iface_link(const std::string &name)
{
	int sock;
	struct ifreq ifr;
	struct ethtool_value cmd;
	int link;

	memset(&ifr, 0, sizeof(struct ifreq));

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock<0)
		return 0;

	pt_strcpy(ifr.ifr_name, name.c_str());

	memset(&cmd, 0, sizeof(cmd));

	cmd.cmd = ETHTOOL_GLINK;
	ifr.ifr_data = (caddr_t)&cmd;
        ioctl(sock, SIOCETHTOOL, &ifr);
	close(sock);

	link = cmd.data;

	return link;
}


static int iface_speed(const std::string &name)
{
	int sock;
	struct ifreq ifr;
	struct ethtool_cmd cmd;
	int speed;

	memset(&ifr, 0, sizeof(struct ifreq));

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock<0)
		return 0;

	pt_strcpy(ifr.ifr_name, name.c_str());

	memset(&cmd, 0, sizeof(cmd));

	cmd.cmd = ETHTOOL_GSET;
	ifr.ifr_data = (caddr_t)&cmd;
        ioctl(sock, SIOCETHTOOL, &ifr);
	close(sock);

	speed = ethtool_cmd_speed(&cmd);


	if (speed > 0 && speed <= 100)
		speed = 100;
	if (speed > 100 && speed <= 1000)
		speed = 1000;
	if (speed == 65535 || !iface_link(name))
		speed = 0; /* no link */

	return speed;
}

void network::start_measurement(void)
{
	start_up = 1;
	start_speed = 0;
	end_up = 1;
	end_speed = 0;

	start_speed = iface_speed(name.c_str());

	start_up = net_iface_up(name.c_str());

	do_proc_net_dev();
	start_pkts = pkts;

	before = pt_gettime();
}


void network::end_measurement(void)
{
	int u_100, u_1000, u_high, u_powerunsave;

	after = pt_gettime();

	end_speed = iface_speed(name.c_str());
	end_up = net_iface_up(name.c_str());
	do_proc_net_dev();
	end_pkts = pkts;

	duration = (after.tv_sec - before.tv_sec) + (after.tv_usec - before.tv_usec) / 1000000.0;

	u_100 = 0;
	u_1000 = 0;
	u_high = 0;

	if (start_speed == 100)
		u_100 += 50;
	if (start_speed == 1000)
		u_1000 += 50;
	if (start_speed > 1000)
		u_high += 50;
	if (end_speed == 100)
		u_100 += 50;
	if (end_speed == 1000)
		u_1000 += 50;
	if (end_speed > 1000)
		u_high += 50;

	if (start_pkts > end_pkts)
		end_pkts = start_pkts;

	u_powerunsave = 100 - 100 * get_wifi_power_saving(name.c_str());

	report_utilization(rindex_link_100, u_100);
	report_utilization(rindex_link_1000, u_1000);
	report_utilization(rindex_link_high, u_high);
	report_utilization(rindex_up, (start_up+end_up) / 2.0);
	report_utilization(rindex_pkts, (end_pkts - start_pkts)/(duration + 0.001));
	report_utilization(rindex_powerunsave, u_powerunsave);
}


double network::utilization(void)
{
	return (end_pkts - start_pkts) / (duration + 0.001);
}

static void netdev_callback(const std::string &d_name)
{
	std::string f_name("/sys/class/net/");
	if (d_name == "lo")
		return;

	f_name.append(d_name);

	register_parameter(std::format("{}-up", d_name));
	register_parameter(std::format("{}-powerunsave", d_name));
	register_parameter(std::format("{}-link-100", d_name));
	register_parameter(std::format("{}-link-1000", d_name));
	register_parameter(std::format("{}-link-high", d_name));
	register_parameter(std::format("{}-packets", d_name));

	network *bl = new(std::nothrow) class network(d_name, f_name);
	if (bl) {
		all_devices.push_back(bl);
		nics[d_name] = bl;
	}
}

void create_all_nics(callback fn)
{
	if (!fn)
		fn = &netdev_callback;
	process_directory("/sys/class/net/", fn);
}

double network::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;

	power = 0;
	factor = get_parameter_value(index_up, bundle);
	util = get_result_value(rindex_up, result);

	power += util * factor;

	if (valid_100 == -1) {
		valid_100 = utilization_power_valid(rindex_link_100);
		valid_1000 = utilization_power_valid(rindex_link_1000);
		valid_high = utilization_power_valid(rindex_link_high);
		valid_powerunsave = utilization_power_valid(rindex_powerunsave);
	}

	if (valid_100 > 0) {
		factor = get_parameter_value(index_link_100, bundle);
		util = get_result_value(rindex_link_100, result);
		power += util * factor / 100;
	}


	if (valid_1000 > 0) {
		factor = get_parameter_value(index_link_1000, bundle);
		util = get_result_value(rindex_link_1000, result);
		power += util * factor / 100;
	}

	if (valid_high > 0) {
		factor = get_parameter_value(index_link_high, bundle);
		util = get_result_value(rindex_link_high, result);
		power += util * factor / 100;
	}

	if (valid_powerunsave > 0) {
		factor = get_parameter_value(index_powerunsave, bundle);
		util = get_result_value(rindex_powerunsave, result);
		power += util * factor / 100;
	}

	factor = get_parameter_value(index_pkts, bundle);
	util = get_result_value(rindex_pkts, result);
	if (util > 5000)
		util = 5000;

	power += util * factor / 100;

	return power;
}
