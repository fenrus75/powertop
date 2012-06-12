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
#include <algorithm>

#include "calibrate.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <sys/types.h>
#include <dirent.h>

#include "../parameters/parameters.h"
extern "C" {
#include "../tuning/iw.h"
}

#include <map>
#include <vector>
#include <string>

using namespace std;


static vector<string> usb_devices;
static vector<string> rfkill_devices;
static vector<string> backlight_devices;
static vector<string> scsi_link_devices;
static int blmax;

static map<string, string> saved_sysfs;


static volatile int stop_measurement;

static int wireless_PS;


static void save_sysfs(const char *filename)
{
	char line[4096];
	ifstream file;

	file.open(filename, ios::in);
	if (!file)
		return;
	file.getline(line, 4096);
	file.close();

	saved_sysfs[filename] = line;
}

static void restore_all_sysfs(void)
{
	map<string, string>::iterator it;

	for (it = saved_sysfs.begin(); it != saved_sysfs.end(); it++)
		write_sysfs(it->first, it->second);

	set_wifi_power_saving("wlan0", wireless_PS);
}

static void find_all_usb(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];

	dir = opendir("/sys/bus/usb/devices/");
	if (!dir)
		return;
	while (1) {
		ifstream file;

		entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		sprintf(filename, "/sys/bus/usb/devices/%s/power/active_duration", entry->d_name);
		if (access(filename, R_OK)!=0)
			continue;

		sprintf(filename, "/sys/bus/usb/devices/%s/power/idVendor", entry->d_name);
		file.open(filename, ios::in);
		if (file) {
			file.getline(filename, 4096);
			file.close();
			if (strcmp(filename, "1d6b")==0)
				continue;
		}

		sprintf(filename, "/sys/bus/usb/devices/%s/power/control", entry->d_name);

		save_sysfs(filename);

		usb_devices.push_back(filename);
	}
	closedir(dir);
}

static void suspend_all_usb_devices(void)
{
	unsigned int i;

	for (i = 0; i < usb_devices.size(); i++)
		write_sysfs(usb_devices[i], "auto\n");
}


static void find_all_rfkill(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];

	dir = opendir("/sys/class/rfkill/");
	if (!dir)
		return;
	while (1) {
		entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		sprintf(filename, "/sys/class/rfkill/%s/soft", entry->d_name);
		if (access(filename, R_OK)!=0)
			continue;

		save_sysfs(filename);

		rfkill_devices.push_back(filename);
	}
	closedir(dir);
}

static void rfkill_all_radios(void)
{
	unsigned int i;

	for (i = 0; i < rfkill_devices.size(); i++)
		write_sysfs(rfkill_devices[i], "1\n");
}
static void unrfkill_all_radios(void)
{
	unsigned int i;

	for (i = 0; i < rfkill_devices.size(); i++)
		write_sysfs(rfkill_devices[i], "0\n");
}

static void find_backlight(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];

	dir = opendir("/sys/class/backlight/");
	if (!dir)
		return;
	while (1) {
		entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		sprintf(filename, "/sys/class/backlight/%s/brightness", entry->d_name);
		if (access(filename, R_OK)!=0)
			continue;

		save_sysfs(filename);

		backlight_devices.push_back(filename);

		sprintf(filename, "/sys/class/backlight/%s/max_brightness", entry->d_name);
		blmax = read_sysfs(filename);
	}
	closedir(dir);
}

static void lower_backlight(void)
{
	unsigned int i;

	for (i = 0; i < backlight_devices.size(); i++)
		write_sysfs(backlight_devices[i], "0\n");
}


static void find_scsi_link(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];

	dir = opendir("/sys/class/scsi_host/");
	if (!dir)
		return;
	while (1) {
		entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		sprintf(filename, "/sys/class/scsi_host/%s/link_power_management_policy", entry->d_name);
		if (access(filename, R_OK)!=0)
			continue;

		save_sysfs(filename);

		scsi_link_devices.push_back(filename);

	}
	closedir(dir);
}

static void set_scsi_link(const char *state)
{
	unsigned int i;

	for (i = 0; i < scsi_link_devices.size(); i++)
		write_sysfs(scsi_link_devices[i], state);
}


static void *burn_cpu(void *dummy)
{
	volatile double d = 1.1;

	while (!stop_measurement) {
		d = pow(d, 1.0001);
	}
	return NULL;
}

static void *burn_cpu_wakeups(void *dummy)
{
	struct timespec tm;

	while (!stop_measurement) {
		tm.tv_sec = 0;
		tm.tv_nsec = (unsigned long)dummy;
		nanosleep(&tm, NULL);
	}
	return NULL;
}

static void *burn_disk(void *dummy)
{
	int fd;
	char buffer[64*1024];
	char filename[256];

	strcpy(filename ,"/tmp/powertop.XXXXXX");
	fd = mkstemp(filename);

	if (fd < 0) {
		printf(_("Cannot create temp file\n"));
		return NULL;
	}

	while (!stop_measurement) {
		lseek(fd, 0, SEEK_SET);
		write(fd, buffer, 64*1024);
		fdatasync(fd);
	}
	close(fd);
	return NULL;
}


static void cpu_calibration(int threads)
{
	int i;
	pthread_t thr;

	printf(_("Calibrating: CPU usage on %i threads\n"), threads);

	stop_measurement = 0;
	for (i = 0; i < threads; i++)
		pthread_create(&thr, NULL, burn_cpu, NULL);

	one_measurement(15, NULL);
	stop_measurement = 1;
	sleep(1);
}

static void wakeup_calibration(unsigned long interval)
{
	pthread_t thr;

	printf(_("Calibrating: CPU wakeup power consumption\n"));

	stop_measurement = 0;

	pthread_create(&thr, NULL, burn_cpu_wakeups, (void *)interval);

	one_measurement(15, NULL);
	stop_measurement = 1;
	sleep(1);
}

static void usb_calibration(void)
{
	unsigned int i;

	/* chances are one of the USB devices is bluetooth; unrfkill first */
	unrfkill_all_radios();
	printf(_("Calibrating USB devices\n"));
	for (i = 0; i < usb_devices.size(); i++) {
		printf(_(".... device %s \n"), usb_devices[i].c_str());
		suspend_all_usb_devices();
		write_sysfs(usb_devices[i], "on\n");
		one_measurement(15, NULL);
		suspend_all_usb_devices();
		sleep(3);
	}
	rfkill_all_radios();
	sleep(4);
}

static void rfkill_calibration(void)
{
	unsigned int i;

	printf(_("Calibrating radio devices\n"));
	for (i = 0; i < rfkill_devices.size(); i++) {
		printf(_(".... device %s \n"), rfkill_devices[i].c_str());
		rfkill_all_radios();
		write_sysfs(rfkill_devices[i], "0\n");
		one_measurement(15, NULL);
		rfkill_all_radios();
		sleep(3);
	}
	for (i = 0; i < rfkill_devices.size(); i++) {
		printf(_(".... device %s \n"), rfkill_devices[i].c_str());
		unrfkill_all_radios();
		write_sysfs(rfkill_devices[i], "1\n");
		one_measurement(15, NULL);
		unrfkill_all_radios();
		sleep(3);
	}
	rfkill_all_radios();
}

static void backlight_calibration(void)
{
	unsigned int i;

	printf(_("Calibrating backlight\n"));
	for (i = 0; i < backlight_devices.size(); i++) {
		char str[4096];
		printf(_(".... device %s \n"), backlight_devices[i].c_str());
		lower_backlight();
		one_measurement(15, NULL);
		sprintf(str, "%i\n", blmax / 4);
		write_sysfs(backlight_devices[i], str);
		one_measurement(15, NULL);

		sprintf(str, "%i\n", blmax / 2);
		write_sysfs(backlight_devices[i], str);
		one_measurement(15, NULL);

		sprintf(str, "%i\n", 3 * blmax / 4 );
		write_sysfs(backlight_devices[i], str);
		one_measurement(15, NULL);

		sprintf(str, "%i\n", blmax);
		write_sysfs(backlight_devices[i], str);
		one_measurement(15, NULL);
		lower_backlight();
		sleep(1);
	}
	printf(_("Calibrating idle\n"));
	system("DISPLAY=:0 /usr/bin/xset dpms force off");
	one_measurement(15, NULL);
	system("DISPLAY=:0 /usr/bin/xset dpms force on");
}

static void idle_calibration(void)
{
	printf(_("Calibrating idle\n"));
	system("DISPLAY=:0 /usr/bin/xset dpms force off");
	one_measurement(15, NULL);
	system("DISPLAY=:0 /usr/bin/xset dpms force on");
}


static void disk_calibration(void)
{
	pthread_t thr;

	printf(_("Calibrating: disk usage \n"));

	set_scsi_link("min_power");

	stop_measurement = 0;
	pthread_create(&thr, NULL, burn_disk, NULL);

	one_measurement(15, NULL);
	stop_measurement = 1;
	sleep(1);


}


void calibrate(void)
{
	find_all_usb();
	find_all_rfkill();
	find_backlight();
	find_scsi_link();
	wireless_PS = get_wifi_power_saving("wlan0");

        save_sysfs("/sys/module/snd_hda_intel/parameters/power_save");

	cout << _("Starting PowerTOP power estimate calibration \n");
	suspend_all_usb_devices();
	rfkill_all_radios();
	lower_backlight();
	set_wifi_power_saving("wlan0", 1);

	sleep(4);


	idle_calibration();
	disk_calibration();
	backlight_calibration();

	write_sysfs("/sys/module/snd_hda_intel/parameters/power_save", "1\n");
	cpu_calibration(1);
	cpu_calibration(4);
	wakeup_calibration(10000);
	wakeup_calibration(100000);
	wakeup_calibration(1000000);
	set_wifi_power_saving("wlan0", 0);
	usb_calibration();
	rfkill_calibration();

	cout << _("Finishing PowerTOP power estimate calibration \n");

	restore_all_sysfs();
        learn_parameters(300, 1);
	printf(_("Parameters after calibration:\n"));
	dump_parameter_bundle();
	save_parameters("saved_parameters.powertop");
        save_all_results("saved_results.powertop");

}
