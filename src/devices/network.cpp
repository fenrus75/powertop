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
#include <dirent.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/ethtool.h>

using namespace std;

#include "device.h"
#include "network.h"
#include "../parameters/parameters.h"
#include "../process/process.h"
extern "C" {
#include "../tuning/iw.h"
}

#include <string.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <unistd.h>

static map<string, class network *> nics;

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

static void do_proc_net_dev(void)
{
	static time_t last_time;
	class network *dev;
	ifstream file;
	char line[4096];
	char *c, *c2;

	if (time(NULL) == last_time)
		return;

	last_time = time(NULL);

	file.open("/proc/net/dev", ios::in);
	if (!file)
		return;

	file.getline(line, 4096);
	file.getline(line, 4096);

	while (file) {
		int i = 0;
		unsigned long val = 0;
		uint64_t pkt = 0;
		file.getline(line, 4096);
		c = strchr(line, ':');
		if (!c)
			continue;
		*c = 0;
		c2 = c +1;
		c = line; while (c && *c == ' ') c++;
		/* c now points to the name of the nic */

		dev = nics[c];
		if (!dev)
			continue;

		c = c2++;
		while (c != c2 && strlen(c) > 0) {
			c2 = c;
			val = strtoull(c, &c, 10);
			i++;
			if (i == 1 || i == 10)
				pkt += val;

		}
		dev->pkts = pkt;
	}
	file.close();
}


network::network(const char *_name, const char *path): device()
{
	char line[4096];
	std::string filename(path);
	char devname[128];
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

	strncpy(sysfs_path, path, sizeof(sysfs_path));
	register_sysfs_path(sysfs_path);
	sprintf(devname, "%s", _name);
	sprintf(humanname, "nic:%s", _name);
	strncpy(name, devname, sizeof(name));

	sprintf(devname, "%s-up", _name);
	index_up = get_param_index(devname);
	rindex_up = get_result_index(devname);

	sprintf(devname, "%s-powerunsave", _name);
	index_powerunsave = get_param_index(devname);
	rindex_powerunsave = get_result_index(devname);

	sprintf(devname, "%s-link-100", _name);
	index_link_100 = get_param_index(devname);
	rindex_link_100 = get_result_index(devname);

	sprintf(devname, "%s-link-1000", _name);
	index_link_1000 = get_param_index(devname);
	rindex_link_1000 = get_result_index(devname);

	sprintf(devname, "%s-link-high", _name);
	index_link_high = get_param_index(devname);
	rindex_link_high = get_result_index(devname);

	sprintf(devname, "%s-packets", _name);
	index_pkts = get_param_index(devname);
	rindex_pkts = get_result_index(devname);

	memset(line, 0, 4096);
	filename.append("/device/driver");
	if (readlink(filename.c_str(), line, 4096) > 0) {
		sprintf(humanname, _("Network interface: %s (%s)"), _name,  basename(line));
	};
}

static int net_iface_up(const char *iface)
{
	int sock;
	struct ifreq ifr;
	int ret;

	memset(&ifr, 0, sizeof(struct ifreq));

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock<0)
		return 0;

	strcpy(ifr.ifr_name, iface);

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

static int iface_link(const char *name)
{
	int sock;
	struct ifreq ifr;
	struct ethtool_value cmd;
	int link;

	memset(&ifr, 0, sizeof(struct ifreq));

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock<0)
		return 0;

	strcpy(ifr.ifr_name, name);

	memset(&cmd, 0, sizeof(cmd));

	cmd.cmd = ETHTOOL_GLINK;
	ifr.ifr_data = (caddr_t)&cmd;
        ioctl(sock, SIOCETHTOOL, &ifr);
	close(sock);

	link = cmd.data;

	return link;
}


static int iface_speed(const char *name)
{
	int sock;
	struct ifreq ifr;
	struct ethtool_cmd cmd;
	int speed;

	memset(&ifr, 0, sizeof(struct ifreq));

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock<0)
		return 0;

	strcpy(ifr.ifr_name, name);

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

	start_speed = iface_speed(name);

	start_up = net_iface_up(name);

	do_proc_net_dev();
	start_pkts = pkts;

	gettimeofday(&before, NULL);
}


void network::end_measurement(void)
{
	int u_100, u_1000, u_high, u_powerunsave;

	gettimeofday(&after, NULL);

	end_speed = iface_speed(name);
	end_up = net_iface_up(name);
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

	u_powerunsave = 100 - 100 * get_wifi_power_saving(name);

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

const char * network::device_name(void)
{
	return name;
}

void netdev_callback(const char *d_name)
{
	std::string f_name("/sys/class/net/");
	f_name.append(d_name);

	network *bl = new(std::nothrow) class network(d_name, f_name.c_str());
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
