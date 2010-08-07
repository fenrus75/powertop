#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <stdlib.h>

#include "cpu.h"


static class abstract_cpu system_level;


static class abstract_cpu * new_package(int package, int cpu, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;
	if (strcmp(vendor, "GenuineIntel") == 0) {
		if (family == 6 && model == 26)
			ret = new class nhm_package;
	}

	if (!ret)
		ret = new class cpu_package;

	ret->set_number(package, cpu);
	return ret;
}

static class abstract_cpu * new_core(int core, int cpu, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;

	if (strcmp(vendor, "GenuineIntel") == 0) {
		if (family == 6 && model == 26)
			ret = new class nhm_core;
	}

	if (!ret)
		ret = new class cpu_core;
	ret->set_number(core, cpu);

	return ret;
}

static class abstract_cpu * new_cpu(int number, char * vendor, int family, int model)
{
	class abstract_cpu * ret;

	if (strcmp(vendor, "GenuineIntel") == 0) {
		if (family == 6 && model == 26)
			ret = new class nhm_cpu;
	}

	if (!ret)
		ret = new class cpu_linux;
	ret->set_number(number, number);
	
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


	if (system_level.children.size() <= package_number)
		system_level.children.resize(package_number + 1);

	if (!system_level.children[package_number])
		system_level.children[package_number] = new_package(package_number, number, vendor, family, model);

	package = system_level.children[package_number];

	if (package->children.size() <= core_number)
		package->children.resize(core_number + 1);

	if (!package->children[core_number])
		package->children[core_number] = new_core(core_number, number, vendor, family, model);

	core = package->children[core_number];

	if (core->children.size() <= number)
		core->children.resize(number + 1, NULL);
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

void start_cpu_measurement(void)
{
	system_level.measurement_start();
}

void end_cpu_measurement(void)
{
	system_level.measurement_end();
}

static void expand_string(char *string, unsigned int newlen)
{
	while (strlen(string) < newlen)
		strcat(string, " ");
}


void display_cpu_cstates(void)
{
	char buffer[128];
	char linebuf[1024];
	unsigned int package, core, cpu;
	int line;
	class abstract_cpu *_package, * _core, * _cpu;
	int ctr = 0;


	for (package = 0; package < system_level.children.size(); package++) {
		int first_pkg = 0;
		_package = system_level.children[package];
		if (!_package)
			continue;
		

		for (core = 0; core < _package->children.size(); core++) {
			_core = _package->children[core];
			if (!_core)
				continue;

			for (line = LEVEL_HEADER; line < 10; line++) {
				ctr = 22;
				int first = 1;
				linebuf[0] = 0;
				if (!_package->has_cstate_level(line))
					continue;
	
				buffer[0] = 0;
				if (first_pkg == 0) {
					strcat(linebuf, _package->fill_cstate_name(line, buffer));
					expand_string(linebuf, 10);
					strcat(linebuf, _package->fill_cstate_line(line, buffer));
				}
				expand_string(linebuf, 20);
	
				strcat(linebuf, "| ");


				buffer[0] = 0;
				strcat(linebuf, _core->fill_cstate_name(line, buffer));
				expand_string(linebuf, ctr + 10);
				strcat(linebuf, _core->fill_cstate_line(line, buffer));
				ctr += 20;
				expand_string(linebuf, ctr);

				strcat(linebuf, "| ");
				ctr += 2;

				for (cpu = 0; cpu < _core->children.size(); cpu++) {
					_cpu = _core->children[cpu];
					if (!_cpu)
						continue;

					if (first == 1) {
						strcat(linebuf, _cpu->fill_cstate_name(line, buffer));
						expand_string(linebuf, ctr + 10);
						first = 0;
						ctr += 12;
					}
					buffer[0] = 0;
					strcat(linebuf, _cpu->fill_cstate_line(line, buffer));
					ctr += 18;
					expand_string(linebuf, ctr);

				}
				strcat(linebuf, "| ");
				printf("%s \n", linebuf);
			}

			printf("\n");
			first_pkg++;
		}


	}
		
}


void display_cpu_pstates(void)
{
	char buffer[128];
	char linebuf[1024];
	unsigned int package, core, cpu;
	int line;
	class abstract_cpu *_package, * _core, * _cpu;
	int ctr = 0;


	for (package = 0; package < system_level.children.size(); package++) {
		int first_pkg = 0;
		_package = system_level.children[package];
		if (!_package)
			continue;
		

		for (core = 0; core < _package->children.size(); core++) {
			_core = _package->children[core];
			if (!_core)
				continue;

			for (line = LEVEL_HEADER; line < 10; line++) {
				ctr = 22;
				int first = 1;
				linebuf[0] = 0;
//				if (!_package->has_pstate_level(line))
//					continue;
	
				buffer[0] = 0;
				if (first_pkg == 0) {
					strcat(linebuf, _package->fill_pstate_name(line, buffer));
					expand_string(linebuf, 10);
					strcat(linebuf, _package->fill_pstate_line(line, buffer));
				}
				expand_string(linebuf, 20);
	
				strcat(linebuf, "| ");


				buffer[0] = 0;
				strcat(linebuf, _core->fill_pstate_name(line, buffer));
				expand_string(linebuf, ctr + 10);
				strcat(linebuf, _core->fill_pstate_line(line, buffer));
				ctr += 20;
				expand_string(linebuf, ctr);

				strcat(linebuf, "| ");
				ctr += 2;

				for (cpu = 0; cpu < _core->children.size(); cpu++) {
					_cpu = _core->children[cpu];
					if (!_cpu)
						continue;

					if (first == 1) {
						strcat(linebuf, _cpu->fill_pstate_name(line, buffer));
						expand_string(linebuf, ctr + 10);
						first = 0;
						ctr += 12;
					}
					buffer[0] = 0;
					strcat(linebuf, _cpu->fill_pstate_line(line, buffer));
					ctr += 8;
					expand_string(linebuf, ctr);

				}
				strcat(linebuf, "| ");
				printf("%s \n", linebuf);
			}

			printf("\n");
			first_pkg++;
		}


	}
		
}


