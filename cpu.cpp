#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <stdlib.h>

#include "cpu.h"


vector<class abstract_cpu *> all_packages;


static class abstract_cpu * new_package(int package, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;
	if (strcmp(vendor, "GenuineIntel") == 0) {
		if (family == 6 && model == 26)
			ret = new class nhm_package;
	}

	if (!ret)
		ret = new class cpu_package;

	ret->set_number(package);
	return ret;
}

static class abstract_cpu * new_core(int core, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;

	if (strcmp(vendor, "GenuineIntel") == 0) {
		if (family == 6 && model == 26)
			ret = new class nhm_core;
	}

	if (!ret)
		ret = new class cpu_core;
	ret->set_number(core);

	return ret;
}

static class abstract_cpu * new_cpu(int number, char * vendor, int family, int model)
{
	class abstract_cpu * ret;

	ret = new class cpu_linux;
	ret->set_number(number);
	
	return ret;
}


	

static void handle_one_cpu(unsigned int number, char *vendor, int family, int model)
{
	char filename[1024];
	ifstream file;
	unsigned int package_number = 0;
	unsigned int core_number = 0;
	class abstract_cpu *package, *core, *cpu;

	sprintf(filename, "/sys/devices/system/cpu/cpu%i/topology/core_id", number);
	file.open(filename, ios::in);
	if (file) {
		file >> core_number;
		file.close();
	}

	sprintf(filename, "/sys/devices/system/cpu/cpu%i/topology/physical_package_id", number);
	file.open(filename, ios::in);
	if (file) {
		file >> package_number;
		file.close();
	}


	if (all_packages.size() <= package_number)
		all_packages.resize(package_number + 1);

	if (!all_packages[package_number])
		all_packages[package_number] = new_package(package_number, vendor, family, model);

	package = all_packages[package_number];

	if (package->children.size() <= core_number)
		package->children.resize(core_number + 1);

	if (!package->children[core_number])
		package->children[core_number] = new_core(core_number, vendor, family, model);

	core = package->children[core_number];

	if (core->children.size() <= number)
		core->children.resize(number + 1);
	if (!core->children[number])
		core->children[number] = new_cpu(number, vendor, family, model);

	cpu = core->children[number];	

}

void enumerate_cpus(void)
{
	ifstream file;	
	char line[1024];

	int number = -1;
	char vendor[128];
	int family = 0;
	int model = 0;

	file.open("/proc/cpuinfo",  ios::in);

	if (!file)
		return;

	while (file) {

		file.getline(line, sizeof(line));
		if (strncmp(line, "vendor_id\t",10) == 0) {
			char *c;
			c = strchr(line, ':');
			if (c) {
				c++;
				if (*c == ' ')
					c++;
				strncpy(vendor,c, 127);
			}
		}
		if (strncmp(line, "processor\t",10) == 0) {
			char *c;
			c = strchr(line, ':');
			if (c) {
				c++;
				number = strtoull(c, NULL, 10);
			}
		}
		if (strncmp(line, "cpu family\t",11) == 0) {
			char *c;
			c = strchr(line, ':');
			if (c) {
				c++;
				family = strtoull(c, NULL, 10);
			}
		}
		if (strncmp(line, "model\t",6) == 0) {
			char *c;
			c = strchr(line, ':');
			if (c) {
				c++;
				model = strtoull(c, NULL, 10);
			}
			handle_one_cpu(number, vendor, family, model);
		}
	}


	file.close();
}

void display_cpus(void)
{
	unsigned int i = 0;
	while (i < all_packages.size()) {
		if (all_packages[i])
			all_packages[i]->display();
		i++;
	}

}