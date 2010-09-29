#include <iostream>
#include <fstream>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>


using namespace std;

#include "device.h"
#include "network.h"
#include "../parameters/parameters.h"

#include <string.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>


network::network(char *_name, char *path)
{
	char line[4096];
	char filename[4096];
	char devname[128];
	start_up = 0;
	end_up = 0;
	start_link = 0;
	end_link = 0;
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

	report_utilization(rindex_link, (start_link+end_link) / 2.0);
	report_utilization(rindex_up, (start_up+end_up) / 2.0);
}


double network::utilization(void)
{
	double p;
	p = 25 * (start_up + start_link + end_up + end_link);

	return p;
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

	return power;
}