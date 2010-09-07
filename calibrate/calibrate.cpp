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

#include <map>
#include <vector>
#include <string>

using namespace std;


static vector<string> usb_devices;
static vector<string> rfkill_devices;
static vector<string> backlight_devices;
static int blmax;

static map<string, string> saved_sysfs;


static volatile int stop_measurement;


static void save_sysfs(char *filename)
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

static void write_sysfs(string filename, string value)
{
	ofstream file;

	file.open(filename.c_str(), ios::out);
	if (!file)
		return;
	file << value;
	file.close();
}

static int read_sysfs(string filename)
{
	ifstream file;
	int i;

	file.open(filename.c_str(), ios::in);
	if (!file)
		return 0;
	file >> i;
	file.close();
	return i;
}

static void restore_all_sysfs(void)
{
	map<string, string>::iterator it;

	for (it = saved_sysfs.begin(); it != saved_sysfs.end(); it++)
		write_sysfs(it->first, it->second);
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
		ifstream file;

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
		ifstream file;

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
		printf("Cannot create temp file\n");
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

	printf("Calibrating: CPU usage on %i threads\n", threads);

	stop_measurement = 0;
	for (i = 0; i < threads; i++)
		pthread_create(&thr, NULL, burn_cpu, NULL);

	one_measurement(30);
	stop_measurement = 1;
	sleep(1);
}

static void wakeup_calibration(unsigned long interval)
{	
	pthread_t thr;

	printf("Calibrating: CPU wakeup power consumption\n");

	stop_measurement = 0;
	
	pthread_create(&thr, NULL, burn_cpu_wakeups, (void *)interval);

	one_measurement(20);
	stop_measurement = 1;
	sleep(1);
}

static void usb_calibration(void)
{
	unsigned int i;

	printf("Calibrating USB devices\n");
	for (i = 0; i < usb_devices.size(); i++) {
		printf(".... device %s \n", usb_devices[i].c_str());
		suspend_all_usb_devices();
		write_sysfs(usb_devices[i], "on\n");
		one_measurement(15);
		suspend_all_usb_devices();
		sleep(3);		
	}
}

static void rfkill_calibration(void)
{
	unsigned int i;

	printf("Calibrating radio devices\n");
	for (i = 0; i < rfkill_devices.size(); i++) {
		printf(".... device %s \n", rfkill_devices[i].c_str());
		rfkill_all_radios();
		write_sysfs(rfkill_devices[i], "0\n");
		one_measurement(15);
		rfkill_all_radios();
		sleep(3);		
	}
	for (i = 0; i < rfkill_devices.size(); i++) {
		printf(".... device %s \n", rfkill_devices[i].c_str());
		unrfkill_all_radios();
		write_sysfs(rfkill_devices[i], "1\n");
		one_measurement(15);
		unrfkill_all_radios();
		sleep(3);		
	}
	rfkill_all_radios();
}

static void backlight_calibration(void)
{
	unsigned int i;

	printf("Calibrating backlight\n");
	for (i = 0; i < backlight_devices.size(); i++) {
		char str[4096];
		printf(".... device %s \n", backlight_devices[i].c_str());
		lower_backlight();
		one_measurement(15);
		sprintf(str, "%i\n", blmax / 2);
		write_sysfs(backlight_devices[i], str);
		one_measurement(15);
		sprintf(str, "%i\n", blmax);
		write_sysfs(backlight_devices[i], str);
		one_measurement(15);
		lower_backlight();
		sleep(1);		
	}
	system("DISPLAY=:0 /usr/bin/xset dpms force off");	
	one_measurement(15);
	system("DISPLAY=:0 /usr/bin/xset dpms force on");	
}


static void disk_calibration(void)
{	
	pthread_t thr;

	printf("Calibrating: disk usage \n");

	stop_measurement = 0;
	pthread_create(&thr, NULL, burn_disk, NULL);

	one_measurement(20);
	stop_measurement = 1;
	sleep(1);
}


void calibrate(void)
{
	find_all_usb();
	find_all_rfkill();
	find_backlight();

	cout << "Starting PowerTOP power estimate calibration \n";
	suspend_all_usb_devices();
	rfkill_all_radios();
	lower_backlight();
	

	backlight_calibration();
	cpu_calibration(1);
	cpu_calibration(4);
	wakeup_calibration(10000);
	wakeup_calibration(100000);
	wakeup_calibration(1000000);
	usb_calibration();
	rfkill_calibration();
	disk_calibration();

	cout << "Finishing PowerTOP power estimate calibration \n";

	restore_all_sysfs();
        learn_parameters(60);
        learn_parameters(100);
        learn_parameters(400);
	printf("Parameters after calibration:\n");
	dump_parameter_bundle();
	save_parameters("saved_parameters.powertop");
        save_all_results("saved_results.powertop");

}