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
#include <errno.h>
#include <limits.h>

#include "../parameters/parameters.h"
extern "C" {
#include "../tuning/iw.h"
}

#include <map>
#include <vector>
#include <string>


static vector<string> usb_devices;
static vector<string> rfkill_devices;
static vector<string> backlight_devices;
static vector<string> scsi_link_devices;
static int blmax;

static map<string, string> saved_sysfs;


static volatile int stop_measurement;

static int wireless_PS;


static void save_sysfs(const std::string &filename)
{
	saved_sysfs[filename] = read_sysfs_string(filename);
}

static void restore_all_sysfs(void)
{
	map<string, string>::iterator it;

	for (it = saved_sysfs.begin(); it != saved_sysfs.end(); it++)
		write_sysfs(it->first, it->second);

	set_wifi_power_saving("wlan0", wireless_PS);
}

static void find_all_usb_callback(const std::string &d_name)
{
	std::string filename;

	filename = std::format("/sys/bus/usb/devices/{}/power/active_duration", d_name);
	if (access(filename.c_str(), R_OK) != 0)
		return;

	std::string vendor_id = read_file_content(std::format("/sys/bus/usb/devices/{}/idVendor", d_name));
	if (!vendor_id.empty()) {
		vendor_id.erase(std::remove(vendor_id.begin(), vendor_id.end(), '\n'), vendor_id.end());
		if (vendor_id == "1d6b")
			return;
	}

	filename = std::format("/sys/bus/usb/devices/{}/power/control", d_name);
	save_sysfs(filename);
	usb_devices.push_back(filename);
}

static void find_all_usb(void)
{
	process_directory("/sys/bus/usb/devices/", find_all_usb_callback);
}

static void suspend_all_usb_devices(void)
{
	unsigned int i;

	for (i = 0; i < usb_devices.size(); i++)
		write_sysfs(usb_devices[i], "auto\n");
}

static void find_all_rfkill_callback(const std::string &d_name)
{
	std::string filename;
	filename = std::format("/sys/class/rfkill/{}/soft", d_name);
	if (access(filename.c_str(), R_OK) != 0)
		return;
	save_sysfs(filename);
	rfkill_devices.push_back(filename);
}

static void find_all_rfkill(void)
{
	process_directory("/sys/class/rfkill/", find_all_rfkill_callback);
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

static void find_backlight_callback(const std::string &d_name)
{
	std::string filename;
	filename = std::format("/sys/class/backlight/{}/brightness", d_name);
	if (access(filename.c_str(), R_OK) != 0)
		return;

	save_sysfs(filename);
	backlight_devices.push_back(filename);
	filename = std::format("/sys/class/backlight/{}/max_brightness", d_name);
	blmax = read_sysfs(filename);
}

static void find_backlight(void)
{
	process_directory("/sys/class/backlight/", find_backlight_callback);
}

static void lower_backlight(void)
{
	unsigned int i;

	for (i = 0; i < backlight_devices.size(); i++)
		write_sysfs(backlight_devices[i], "0\n");
}

static void find_scsi_link_callback(const std::string &d_name)
{
	std::string filename;
	filename = std::format("/sys/class/scsi_host/{}/link_power_management_policy", d_name);
	if (access(filename.c_str(), R_OK)!=0)
		return;

	save_sysfs(filename);
	scsi_link_devices.push_back(filename);
}

static void find_scsi_link(void)
{
	process_directory("/sys/class/scsi_host/", find_scsi_link_callback);
}

static void set_scsi_link(const std::string &state)
{
	unsigned int i;

	for (i = 0; i < scsi_link_devices.size(); i++)
		write_sysfs(scsi_link_devices[i], state);
}


static void *burn_cpu(void *dummy __unused)
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

static void *burn_disk(void *dummy __unused)
{
	int fd;
	char buffer[64*1024];
	char filename[] = "/tmp/powertop.XXXXXX";

	fd = mkstemp(filename);

	if (fd < 0) {
		printf(_("Cannot create temp file\n"));
		return NULL;
	}

	while (!stop_measurement) {
		lseek(fd, 0, SEEK_SET);
		if(write(fd, buffer, 64*1024) == -1)
			printf("Error: %s\n", strerror(errno));
		fdatasync(fd);
	}
	unlink(filename);
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

	one_measurement(15, 15, "");
	stop_measurement = 1;
	sleep(1);
}

static void wakeup_calibration(unsigned long interval)
{
	pthread_t thr;

	printf(_("Calibrating: CPU wakeup power consumption\n"));

	stop_measurement = 0;

	pthread_create(&thr, NULL, burn_cpu_wakeups, (void *)interval);

	one_measurement(15, 15, "");
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
		one_measurement(15, 15, "");
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
		one_measurement(15, 15, "");
		rfkill_all_radios();
		sleep(3);
	}
	for (i = 0; i < rfkill_devices.size(); i++) {
		printf(_(".... device %s \n"), rfkill_devices[i].c_str());
		unrfkill_all_radios();
		write_sysfs(rfkill_devices[i], "1\n");
		one_measurement(15, 15, "");
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
		std::string str;
		printf(_(".... device %s \n"), backlight_devices[i].c_str());
		lower_backlight();
		one_measurement(15, 15, "");
		str = std::format("{}\n", blmax / 4);
		write_sysfs(backlight_devices[i], str);
		one_measurement(15, 15, "");

		str = std::format("{}\n", blmax / 2);
		write_sysfs(backlight_devices[i], str);
		one_measurement(15, 15, "");

		str = std::format("{}\n", 3 * blmax / 4 );
		write_sysfs(backlight_devices[i], str);
		one_measurement(15, 15, "");

		str = std::format("{}\n", blmax);
		write_sysfs(backlight_devices[i], str);
		one_measurement(15, 15, "");
		lower_backlight();
		sleep(1);
	}
	printf(_("Calibrating idle\n"));
	if(!system("DISPLAY=:0 /usr/bin/xset dpms force off"))
		printf("System is not available\n");
	one_measurement(15, 15, "");
	if(!system("DISPLAY=:0 /usr/bin/xset dpms force on"))
		printf("System is not available\n");
}

static void idle_calibration(void)
{
	printf(_("Calibrating idle\n"));
	if(!system("DISPLAY=:0 /usr/bin/xset dpms force off"))
		printf("System is not available\n");
	one_measurement(15, 15, "");
	if(!system("DISPLAY=:0 /usr/bin/xset dpms force on"))
		printf("System is not available\n");
}


static void disk_calibration(void)
{
	pthread_t thr;

	printf(_("Calibrating: disk usage \n"));

	set_scsi_link("min_power");

	stop_measurement = 0;
	pthread_create(&thr, NULL, burn_disk, NULL);

	one_measurement(15, 15, "");
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
