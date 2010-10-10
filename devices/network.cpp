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


using namespace std;

#include "device.h"
#include "network.h"
#include "../parameters/parameters.h"
#include "../process/process.h"

#include <string.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>


static map<string, class network *> nics;


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


network::network(char *_name, char *path)
{
	char line[4096];
	char filename[4096];
	char devname[128];
	start_up = 0;
	end_up = 0;
	start_link = 0;
	end_link = 0;
	start_pkts = 0;
	end_pkts = 0;
	pkts = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));
	sprintf(devname, "%s", _name);
	sprintf(humanname, "nic:%s", _name);
	strncpy(name, devname, sizeof(name));

	sprintf(devname, "%s-up", _name);
	index_up = get_param_index(devname);
	rindex_up = get_result_index(devname);

	sprintf(devname, "%s-link", _name);
	index_link = get_param_index(devname);
	rindex_link = get_result_index(devname);

	sprintf(devname, "%s-packets", _name);
	index_pkts = get_param_index(devname);
	rindex_pkts = get_result_index(devname);

	memset(line, 0, 4096);
	sprintf(filename, "%s/device/driver", path);
	if (readlink(filename, line, 4096) > 0) {
		sprintf(humanname, "Network interface: %s (%s)",_name,  basename(line));
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

	return 0;
}

void network::start_measurement(void)
{
	char filename[4096];
	ifstream file;

	start_up = 1;
	start_link = 1;
	end_up = 1;
	end_link = 1;

	sprintf(filename, "%s/operstate", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file.getline(filename, 4096);
		if (strcmp(filename, "up")==0)
			start_link = 1;
	}
	file.close();

	start_up = net_iface_up(name);

	do_proc_net_dev();
	start_pkts = pkts;
}


void network::end_measurement(void)
{
	char filename[4096];
	ifstream file;

	sprintf(filename, "%s/operstate", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file.getline(filename, 4096);
		if (strcmp(filename, "up")==0)
			end_link = 1;
	}
	file.close();

	end_up = net_iface_up(name);
	do_proc_net_dev();
	end_pkts = pkts;

	report_utilization(rindex_link, (start_link+end_link) / 2.0);
	report_utilization(rindex_up, (start_up+end_up) / 2.0);
	report_utilization(rindex_pkts, (end_pkts - start_pkts)/(measurement_time + 0.001));
}


double network::utilization(void)
{
	return (end_pkts - start_pkts) / measurement_time;
}

const char * network::device_name(void)
{
	return name;
}

void create_all_nics(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];
	dir = opendir("/sys/class/net/");
	if (!dir)
		return;
	while (1) {
		class network *bl;
		ifstream file;
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;
		if (strcmp(entry->d_name, "lo")==0)
			continue;

		sprintf(filename, "/sys/class/net/%s", entry->d_name);
		bl = new class network(entry->d_name, filename);
		all_devices.push_back(bl);
		nics[entry->d_name] = bl;
	}
	closedir(dir);

}



double network::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double utilization;

	power = 0;
	factor = get_parameter_value(index_up, bundle);
	utilization = get_result_value(rindex_up, result);

	power += utilization * factor;

	factor = get_parameter_value(index_link, bundle);
	utilization = get_result_value(rindex_link, result);

	power += utilization * factor;

	factor = get_parameter_value(index_pkts, bundle);
	utilization = get_result_value(rindex_pkts, result);

	power += utilization * factor / 100;

	return power;
}