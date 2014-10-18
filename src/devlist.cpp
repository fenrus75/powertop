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

/*
 * Code to track centrally which process has what /dev files open
 */
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

using namespace std;

#include "devlist.h"
#include "lib.h"
#include "report/report.h"
#include "report/report-maker.h"
#include "report/report-data-html.h"

#include "process/process.h"
#include "devices/device.h"
/*

* collect list of processes that have devices open
  (alternate between before and after lists)

* charge a "surcharge" to a device (sub)string
  - count how many openers
  - add proprotion to each process that has it open

* list of devices + power they use for processing

*/

static vector<struct devuser *> one;
static vector<struct devuser *> two;
static vector<struct devpower *> devpower;

static int phase;
/*
 * 0 - one = before,  two = after
 * 1 - one = after,   two = before
 */

void clean_open_devices()
{
	unsigned int i=0;

	for (i = 0; i < one.size(); i++) {
		free(one[i]);
	}

	for (i = 0; i < two.size(); i++) {
		free(two[i]);
	}

	for (i = 0; i < devpower.size(); i++){
		free(devpower[i]);
	}
}

void collect_open_devices(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];
	char link[4096];
	unsigned int i;
	vector<struct devuser *> *target;

	if (phase == 1)
		target = &one;
	else
		target = &two;

	for (i = 0; i < target->size(); i++) {
		free((*target)[i]);
	}
	target->resize(0);


	dir = opendir("/proc/");
	if (!dir)
		return;
	while (1) {
		struct dirent *entry2;
		DIR *dir2;
		entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;
		if (strcmp(entry->d_name, "self") == 0)
			continue;

		sprintf(filename, "/proc/%s/fd/", entry->d_name);

		dir2 = opendir(filename);
		if (!dir2)
			continue;
		while (1) {
			int ret;
			struct devuser * dev;
			entry2 = readdir(dir2);
			if (!entry2)
				break;
			if (!isdigit(entry2->d_name[0]))
				continue;
			sprintf(filename, "/proc/%s/fd/%s", entry->d_name, entry2->d_name);
			memset(link, 0, 4096);
			ret = readlink(filename, link, 4095);
			if (ret < 0)
				continue;

			if (strcmp(link, "/dev/null") == 0)
				continue;
			if (strcmp(link, "/dev/.udev/queue.bin") == 0)
				continue;
			if (strcmp(link, "/dev/initctl") == 0)
				continue;
			if (strcmp(link, "/dev/ptmx") == 0)
				continue;
			if (strstr(link, "/dev/pts/"))
				continue;
			if (strstr(link, "/dev/shm/"))
				continue;
			if (strstr(link, "/dev/urandom"))
				continue;
			if (strstr(link, "/dev/tty"))
				continue;

			if (strncmp(link, "/dev", 4)==0) {
				dev = (struct devuser *)malloc(sizeof(struct devuser));
				if (!dev)
					continue;
				dev->pid = strtoull(entry->d_name, NULL, 10);
				strncpy(dev->device, link, 251);
				dev->device[251] = '\0';
				strncpy(dev->comm, read_sysfs_string("/proc/%s/comm", entry->d_name).c_str(), 31);
				dev->comm[31] = '\0';
				target->push_back(dev);

			}
		}
		closedir(dir2);
	}
	closedir(dir);

	if (phase)
		phase = 0;
	else
		phase = 1;
}


/* returns 0 if no process is identified as having the device open and a value > 0 otherwise */
int charge_device_to_openers(const char *devstring, double power, class device *_dev)
{
	unsigned int i;
	int openers = 0;
	class process *proc;
	/* 1. count the number of openers */

	for (i = 0; i < one.size(); i++) {
		if (strstr(one[i]->device, devstring))
			openers++;
		}
	for (i = 0; i < two.size(); i++) {
		if (strstr(two[i]->device, devstring))
			openers++;
	}


	/* 2. divide power by this number */

	if (!openers)
		return 0;
	power = power / openers;


	/* 3. for each process that has it open, add the charge */

	for (i = 0; i < one.size(); i++)
		if (strstr(one[i]->device, devstring)) {
			proc = find_create_process(one[i]->comm, one[i]->pid);
			if (proc) {
				proc->power_charge += power;
				if (strlen(_dev->guilty) < 2000 && strstr(_dev->guilty, one[i]->comm) == NULL) {
					strcat(_dev->guilty, one[i]->comm);
					strcat(_dev->guilty, " ");
				}
			}
		}

	for (i = 0; i < two.size(); i++)
		if (strstr(two[i]->device, devstring)) {
			proc = find_create_process(two[i]->comm, two[i]->pid);
			if (proc) {
				proc->power_charge += power;
				if (strlen(_dev->guilty) < 2000 && strstr(_dev->guilty, two[i]->comm) == NULL) {
					strcat(_dev->guilty, two[i]->comm);
					strcat(_dev->guilty, " ");
				}
			}
		}



	return openers;
}

void clear_devpower(void)
{
	unsigned int i;

	for (i = 0; i < devpower.size(); i++) {
		devpower[i]->power = 0.0;
		devpower[i]->dev->guilty[0] = 0;
	}
}

void register_devpower(const char *devstring, double power, class device *_dev)
{
	unsigned int i;
	struct devpower *dev =  NULL;

	for (i = 0; i < devpower.size(); i++)
		if (strcmp(devstring, devpower[i]->device) == 0) {
			dev = devpower[i];
		}

	if (!dev) {
		dev = (struct devpower *)malloc(sizeof (struct devpower));
		strcpy(dev->device, devstring);
		dev->power = 0.0;
		devpower.push_back(dev);
	}
	dev->dev = _dev;
	dev->power = power;
}

void run_devpower_list(void)
{
	unsigned int i;

	for (i = 0; i < devpower.size(); i++) {
		int ret;
		ret = charge_device_to_openers(devpower[i]->device, devpower[i]->power, devpower[i]->dev);
		if (ret)
			devpower[i]->dev->hide = true;
		else
			devpower[i]->dev->hide = false;

	}

}

static bool devlist_sort(struct devuser * i, struct devuser * j)
{
	if (i->pid != j->pid)
		return i->pid < j->pid;

	return (strcmp(i->device, j->device)< 0);
}

void report_show_open_devices(void)
{
	vector<struct devuser *> *target;
	unsigned int i;
	char prev[128], proc[128];
	int idx, cols, rows;

	prev[0] = 0;
	if (phase == 1)
		target = &one;
	else
		target = &two;

	if (target->size() == 0)
		return;


	/* Set Table attributes, rows, and cols */
	table_attributes std_table_css;
	cols = 2;
	idx = cols;
	rows= target->size() + 1;
	init_std_table_attr(&std_table_css, rows, cols);

	/* Set Title attributes */
	tag_attr title_attr;
	init_title_attr(&title_attr);

	/* Set array of data in row Major order */
	string *process_data = new string[cols * rows];

	sort(target->begin(), target->end(), devlist_sort);
	process_data[0]=__("Process");
	process_data[1]=__("Device");

	for (i = 0; i < target->size(); i++) {
		proc[0] = 0;
		if (strcmp(prev, (*target)[i]->comm) != 0)
			sprintf(proc, "%s", (*target)[i]->comm);

		process_data[idx]=string(proc);
		idx+=1;
		process_data[idx]=string((*target)[i]->device);
		idx+=1;
		sprintf(prev, "%s", (*target)[i]->comm);
	}

	/* Report Output */
	/* No div attribute here inherits from device power report */
	report.add_title(&title_attr, __("Process Device Activity"));
	report.add_table(process_data, &std_table_css);
	delete [] process_data;
	report.end_div();
}
