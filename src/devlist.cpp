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
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <cctype>
#include <climits>

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

static std::vector<struct devuser *> one;
static std::vector<struct devuser *> two;
static std::vector<struct devpower *> devpower;

static int phase;
/*
 * 0 - one = before,  two = after
 * 1 - one = after,   two = before
 */

void clean_open_devices()
{
	for (auto *d : one)
		delete d;
	one.clear();

	for (auto *d : two)
		delete d;
	two.clear();

	for (auto *d : devpower)
		delete d;
	devpower.clear();
}

void collect_open_devices(void)
{
	std::vector<struct devuser *> *target;

	if (phase == 1)
		target = &one;
	else
		target = &two;

	for (auto *d : *target)
		delete d;
	target->clear();


	for (const auto &pid : list_directory("/proc/")) {
		if (pid == "self")
			continue;

		for (const auto &fd : list_directory(std::format("/proc/{}/fd/", pid))) {
			if (!isdigit(fd[0]))
				continue;
			const std::string link = pt_readlink(std::format("/proc/{}/fd/{}", pid, fd));
			if (link.empty())
				continue;

			if (link == "/dev/null")
				continue;
			if (link == "/dev/.udev/queue.bin")
				continue;
			if (link == "/dev/initctl")
				continue;
			if (link == "/dev/ptmx")
				continue;
			if (link.find("/dev/pts/") != std::string::npos)
				continue;
			if (link.find("/dev/shm/") != std::string::npos)
				continue;
			if (link.find("/dev/urandom") != std::string::npos)
				continue;
			if (link.find("/dev/tty") != std::string::npos)
				continue;

			if (link.compare(0, 4, "/dev") == 0) {
				struct devuser *dev = new(std::nothrow) struct devuser;
				if (!dev)
					continue;
				dev->pid = strtoull(pid.c_str(), nullptr, 10);
				dev->device = link;
				dev->comm = read_sysfs_string(std::format("/proc/{}/comm", pid));
				target->push_back(dev);
			}
		}
	}

	if (phase)
		phase = 0;
	else
		phase = 1;
}


/* returns 0 if no process is identified as having the device open and a value > 0 otherwise */
int charge_device_to_openers(const std::string &devstring, double power, class device *_dev)
{
	int openers = 0;
	class process *proc;
	/* 1. count the number of openers */

	for (const auto *d : one) {
		if (d->device.find(devstring) != std::string::npos)
			openers++;
	}
	for (const auto *d : two) {
		if (d->device.find(devstring) != std::string::npos)
			openers++;
	}


	/* 2. divide power by this number */

	if (!openers)
		return 0;
	power = power / openers;


	/* 3. for each process that has it open, add the charge */

	for (const auto *d : one)
		if (d->device.find(devstring) != std::string::npos) {
			proc = find_create_process(d->comm, d->pid);
			if (proc) {
				proc->power_charge += power;
				if (_dev->guilty.find(d->comm) == std::string::npos) {
					_dev->guilty += d->comm;
					_dev->guilty += " ";
				}
			}
		}

	for (const auto *d : two)
		if (d->device.find(devstring) != std::string::npos) {
			proc = find_create_process(d->comm, d->pid);
			if (proc) {
				proc->power_charge += power;
				if (_dev->guilty.find(d->comm) == std::string::npos) {
					_dev->guilty += d->comm;
					_dev->guilty += " ";
				}
			}
		}



	return openers;
}

void clear_devpower(void)
{
	for (auto *dp : devpower) {
		dp->power = 0.0;
		dp->dev->guilty.clear();
	}
}

void register_devpower(const std::string &devstring, double power, class device *_dev)
{
	struct devpower *dev =  nullptr;

	for (auto *dp : devpower)
		if (devstring == dp->device)
			dev = dp;

	if (!dev) {
		dev = new(std::nothrow) struct devpower;
		if (!dev)
			return;
		dev->device = devstring;
		dev->power = 0.0;
		devpower.push_back(dev);
	}
	dev->dev = _dev;
	dev->power = power;
}

void run_devpower_list(void)
{
	for (auto *dp : devpower) {
		int ret;
		ret = charge_device_to_openers(dp->device, dp->power, dp->dev);
		if (ret)
			dp->dev->hide = true;
		else
			dp->dev->hide = false;
	}
}

static bool devlist_sort(const devuser *i, const devuser *j)
{
	if (i->pid != j->pid)
		return i->pid < j->pid;

	return i->device < j->device;
}

void report_show_open_devices(void)
{
	std::vector<struct devuser *> *target;
	std::string prev, proc;
	int idx, cols, rows;

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
	std::vector<std::string> process_data(cols * rows);

	sort(target->begin(), target->end(), devlist_sort);
	process_data[0]=__("Process");
	process_data[1]=__("Device");

	for (const auto *d : *target) {
		proc = "";
		if (prev != d->comm)
			proc = d->comm;

		process_data[idx]=proc;
		idx+=1;
		process_data[idx]=d->device;
		idx+=1;
		prev = d->comm;
	}

	/* Report Output */
	/* No div attribute here inherits from device power report */
	report.add_title(&title_attr, __("Process Device Activity"));
	report.add_table(process_data, &std_table_css);
	report.end_div();
}
