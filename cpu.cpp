#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <stdlib.h>

#include "cpu.h"


vector<class abstract_cpu *> all_packages;


static void handle_one_cpu(int number, char *vendor, int family, int model, int stepping)
{
	char filename[1024];
	ifstream file;
	int package_number = 0;
	int core_number = 0;
	cout << "Cpu " << number << " has vendor -" << vendor << "- with family " << family << " model " << model << " stepping " << stepping << "\n";

	sprintf(filename, "/sys/devices/system/cpu/cpu%i/topology/core_id", number);
	file.open(filename, ios::in);
	if (file) {
		file >> core_number;
		file.close();
		cout << "Core is " << core_number << "\n";
	}

	sprintf(filename, "/sys/devices/system/cpu/cpu%i/topology/physical_package_id", number);
	file.open(filename, ios::in);
	if (file) {
		file >> package_number;
		file.close();
		cout << "Package is " << package_number << "\n";
	}

	all_packages[package_number]->measurement_start();
}

void enumerate_cpus(void)
{
	ifstream file;	
	char line[1024];

	int number = -1;
	char vendor[128];
	int family = 0;
	int model = 0;
	int stepping = 0;

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
		}
		if (strncmp(line, "stepping\t",6) == 0) {
			char *c;
			c = strchr(line, ':');
			if (c) {
				c++;
				stepping = strtoull(c, NULL, 10);
			}

			handle_one_cpu(number, vendor, family, model, stepping);
		}
	}


	file.close();
}



