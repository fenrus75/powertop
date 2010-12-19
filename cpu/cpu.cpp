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
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>

#include "cpu.h"
#include "cpudevice.h"
#include "../parameters/parameters.h"

#include "../perf/perf_bundle.h"
#include "../lib.h"
#include "../display.h"
#include "../html.h"

static class abstract_cpu system_level;

vector<class abstract_cpu *> all_cpus;

static	class perf_bundle * perf_events;



class perf_power_bundle: public perf_bundle
{
	virtual void handle_trace_point(int type, void *trace, int cpu, uint64_t time, unsigned char flags);

};


static class abstract_cpu * new_package(int package, int cpu, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;
	class cpudevice *cpudev;
	char packagename[128];
	if (strcmp(vendor, "GenuineIntel") == 0) {
		if (family == 6 && model == 26)
			ret = new class nhm_package;
		if (family == 6 && model == 37)
			ret = new class nhm_package;
	}

	if (!ret)
		ret = new class cpu_package;

	ret->set_number(package, cpu);
	ret->childcount = 0;

	sprintf(packagename, "cpu package %i", cpu);
	cpudev = new class cpudevice("cpu package", packagename, ret);
	all_devices.push_back(cpudev);
	return ret;
}

static class abstract_cpu * new_core(int core, int cpu, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;

	if (strcmp(vendor, "GenuineIntel") == 0) {
		if (family == 6 && model == 26)
			ret = new class nhm_core;
		if (family == 6 && model == 37)
			ret = new class nhm_core;
	}

	if (!ret)
		ret = new class cpu_core;
	ret->set_number(core, cpu);
	ret->childcount = 0;

	return ret;
}

static class abstract_cpu * new_cpu(int number, char * vendor, int family, int model)
{
	class abstract_cpu * ret = NULL;

	if (strcmp(vendor, "GenuineIntel") == 0) {
		if (family == 6 && model == 26)
			ret = new class nhm_cpu;
		if (family == 6 && model == 37)
			ret = new class nhm_cpu;
	}

	if (!ret)
		ret = new class cpu_linux;
	ret->set_number(number, number);
	ret->childcount = 0;
	
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
		system_level.children.resize(package_number + 1, NULL);

	if (!system_level.children[package_number]) {
		system_level.children[package_number] = new_package(package_number, number, vendor, family, model);
		system_level.childcount++;
	}

	package = system_level.children[package_number];
	package->parent = &system_level;

	if (package->children.size() <= core_number)
		package->children.resize(core_number + 1, NULL);

	if (!package->children[core_number]) {
		package->children[core_number] = new_core(core_number, number, vendor, family, model);
		package->childcount++;
	}

	core = package->children[core_number];
	core->parent = package;

	if (core->children.size() <= number)
		core->children.resize(number + 1, NULL);
	if (!core->children[number]) {
		core->children[number] = new_cpu(number, vendor, family, model);
		core->childcount++;
	}

	cpu = core->children[number];
	cpu->parent = core;

	if (number >= all_cpus.size())
		all_cpus.resize(number + 1, NULL);
	all_cpus[number] = cpu;
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
			set_max_cpu(number);
		}
	}


	file.close();

	perf_events = new perf_power_bundle();

	perf_events->add_event("power:power_frequency");
	perf_events->add_event("power:power_start");
	perf_events->add_event("power:power_end");

}

void start_cpu_measurement(void)
{
	perf_events->start();
	system_level.measurement_start();
}

void end_cpu_measurement(void)
{
	system_level.measurement_end();
	perf_events->stop();
}

static void expand_string(char *string, unsigned int newlen)
{
	while (strlen(string) < newlen)
		strcat(string, " ");
}


void display_cpu_cstates(const char *start, const char *end, const char *linestart,
                                const char *separator, const char *lineend)
{
	char buffer[128];
	char linebuf[1024];
	unsigned int package, core, cpu;
	int line;
	class abstract_cpu *_package, * _core, * _cpu;
	int ctr = 0;

	printf("%s", start);		

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
				int first = 1;
				ctr = 0;
				linebuf[0] = 0;
				if (!_package->has_cstate_level(line))
					continue;

				strcat(linebuf, linestart);
				ctr += strlen(linestart);
				
	
				buffer[0] = 0;
				if (first_pkg == 0) {
					strcat(linebuf, _package->fill_cstate_name(line, buffer));
					expand_string(linebuf, ctr+10);
					strcat(linebuf, _package->fill_cstate_line(line, buffer));
				}
				ctr += 20;
				expand_string(linebuf, ctr);
	
				strcat(linebuf, separator);
				ctr += strlen(separator);

				if (!_core->can_collapse()) {
					buffer[0] = 0;
					strcat(linebuf, _core->fill_cstate_name(line, buffer));
					expand_string(linebuf, ctr + 10);
					strcat(linebuf, _core->fill_cstate_line(line, buffer));
					ctr += 20;
					expand_string(linebuf, ctr);

					strcat(linebuf, separator);
					ctr += strlen(separator);
				}

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
				strcat(linebuf, lineend);
				printf("%s", linebuf);
			}

			printf("\n");
			first_pkg++;
		}


	}
	printf("%s", end);		
}

void w_display_cpu_cstates(void)
{
	WINDOW *win;
	char buffer[128];
	char linebuf[1024];
	unsigned int package, core, cpu;
	int line;
	class abstract_cpu *_package, * _core, * _cpu;
	int ctr = 0;

	win = get_ncurses_win("Idle stats");
	if (!win)
		return;

	wclear(win);
        wmove(win, 2,0);

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
				int first = 1;
				ctr = 0;
				linebuf[0] = 0;
				if (!_package->has_cstate_level(line))
					continue;

	
				buffer[0] = 0;
				if (first_pkg == 0) {
					strcat(linebuf, _package->fill_cstate_name(line, buffer));
					expand_string(linebuf, ctr+10);
					strcat(linebuf, _package->fill_cstate_line(line, buffer));
				}
				ctr += 20;
				expand_string(linebuf, ctr);
	
				strcat(linebuf, "| ");
				ctr += strlen("| ");

				if (!_core->can_collapse()) {
					buffer[0] = 0;
					strcat(linebuf, _core->fill_cstate_name(line, buffer));
					expand_string(linebuf, ctr + 10);
					strcat(linebuf, _core->fill_cstate_line(line, buffer));
					ctr += 20;
					expand_string(linebuf, ctr);

					strcat(linebuf, "| ");
					ctr += strlen("| ");
				}

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
				strcat(linebuf, "\n");
				wprintw(win, "%s", linebuf);
			}

			wprintw(win, "\n");
			first_pkg++;
		}


	}
}


void html_display_cpu_pstates(void)
{
	char buffer[512], buffer2[512];
	char linebuf[1024];
	unsigned int package, core, cpu;
	int line;
	class abstract_cpu *_package, * _core, * _cpu;

	if (!htmlout)
		return;


	fprintf(htmlout, "<h2>Processor frequency report</h2>\n");


	for (package = 0; package < system_level.children.size(); package++) {
		int first_pkg = 0;
		_package = system_level.children[package];
		if (!_package)
			continue;
		
		fprintf(htmlout, "<table width=100%%>\n");

		for (core = 0; core < _package->children.size(); core++) {
			_core = _package->children[core];
			if (!_core)
				continue;

			for (line = LEVEL_HEADER; line < 10; line++) {
				int first = 1;
				linebuf[0] = 0;

				if (!_package->has_pstate_level(line))
					continue;


				if (line == LEVEL_HEADER)
					fprintf(htmlout, "<th>");
				else
					fprintf(htmlout, "<tr>");


				buffer[0] = 0;
				buffer2[0] = 0;
				if (first_pkg == 0) {
					if (line == LEVEL_HEADER)
						fprintf(htmlout, "<td colspan=2>%s%s</td>",
							_package->fill_pstate_name(line, buffer),
							_package->fill_pstate_line(line, buffer2));
					else
						fprintf(htmlout, "<td>%s</td><td>%s</td>",
							_package->fill_pstate_name(line, buffer),
							_package->fill_pstate_line(line, buffer2));
				} else {
					fprintf(htmlout, "<td colspan=2>&nbsp;</td>");
				}

				if (!_core->can_collapse()) {
					buffer[0] = 0;
					buffer2[0] = 0;
					if (line == LEVEL_HEADER) 
						fprintf(htmlout, "<td colspan=2>%s%s</td>",
							_core->fill_pstate_name(line, buffer),
							_core->fill_pstate_line(line, buffer2));
					else
						fprintf(htmlout, "<td>%s</td><td>%s</td>",
							_core->fill_pstate_name(line, buffer),
							_core->fill_pstate_line(line, buffer2));
				}

				for (cpu = 0; cpu < _core->children.size(); cpu++) {
					buffer[0] = 0;
					_cpu = _core->children[cpu];
					if (!_cpu)
						continue;

					if (line == LEVEL_HEADER) {
							fprintf(htmlout, "<td colspan=2>%s</td>", _cpu->fill_pstate_line(line, buffer));
					} else {
						if (first == 1) {
							fprintf(htmlout, "<td>%s</td>", _cpu->fill_pstate_name(line, buffer));
							first = 0;
							buffer[0] = 0;
							fprintf(htmlout, "<td>%s</td>",
								_cpu->fill_pstate_line(line, buffer));
						} else {
							buffer[0] = 0;
							fprintf(htmlout, "<td colspan=2>%s</td>",
								_cpu->fill_pstate_line(line, buffer));
						}
					}
				}
				if (line == LEVEL_HEADER)
					fprintf(htmlout, "</th>\n");
				else
					fprintf(htmlout, "</tr>\n");

			}

			first_pkg++;
		}


	}

	fprintf(htmlout, "</table>");		
}

void w_display_cpu_pstates(void)
{
	WINDOW *win;
	char buffer[128];
	char linebuf[1024];
	unsigned int package, core, cpu;
	int line;
	class abstract_cpu *_package, * _core, * _cpu;
	int ctr = 0;

	win = get_ncurses_win("Frequency stats");
	if (!win)
		return;

	wclear(win);
        wmove(win, 2,0);


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
				int first = 1;
				ctr = 0;
				linebuf[0] = 0;

				if (!_package->has_pstate_level(line))
					continue;


				buffer[0] = 0;
				if (first_pkg == 0) {
					strcat(linebuf, _package->fill_pstate_name(line, buffer));
					expand_string(linebuf, ctr + 10);
					strcat(linebuf, _package->fill_pstate_line(line, buffer));
				}
				ctr += 20;
				expand_string(linebuf, ctr);
	
				strcat(linebuf, "| ");
				ctr += strlen("| ");

				if (!_core->can_collapse()) {
					buffer[0] = 0;
					strcat(linebuf, _core->fill_pstate_name(line, buffer));
					expand_string(linebuf, ctr + 10);
					strcat(linebuf, _core->fill_pstate_line(line, buffer));
					ctr += 20;
					expand_string(linebuf, ctr);

					strcat(linebuf, "| ");
					ctr += strlen("| ");
				}

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
					ctr += 10;
					expand_string(linebuf, ctr);

				}
				strcat(linebuf, "\n");
				wprintw(win, "%s", linebuf);
			}

			wprintw(win, "\n");
			first_pkg++;
		}


	}

}



struct power_entry {
#ifdef __x86_64__
	int dummy;
#endif
	int64_t	type;
	int64_t	value;
} __attribute__((packed));


void perf_power_bundle::handle_trace_point(int type, void *trace, int cpunr, uint64_t time, unsigned char flags)
{
	const char *event_name;
	class abstract_cpu *cpu;

	if (type >= (int)event_names.size())
		return;
	event_name = event_names[type];

	if (cpunr >= (int)all_cpus.size()) {
		cout << "INVALID cpu nr in handle_trace_point\n";
		return;
	}

	cpu = all_cpus[cpunr];

#if 0
	unsigned int i;
	printf("Time is %llu \n", time);
	for (i = 0; i < system_level.children.size(); i++)
		if (system_level.children[i])
			system_level.children[i]->validate();
#endif

	if (strcmp(event_name, "power:power_frequency")==0) {
		struct power_entry *pe = (struct power_entry *)trace;
		cpu->change_freq(time, pe->value);
	}
	if (strcmp(event_name, "power:power_start")==0)
		cpu->go_idle(time);
	if (strcmp(event_name, "power:power_end")==0)
		cpu->go_unidle(time);

#if 0
	unsigned int i;
	for (i = 0; i < system_level.children.size(); i++)
		if (system_level.children[i])
			system_level.children[i]->validate();
#endif
}

void process_cpu_data(void)
{
	unsigned int i;
	system_level.reset_pstate_data();

	perf_events->process();

	for (i = 0; i < system_level.children.size(); i++)
		if (system_level.children[i])
			system_level.children[i]->validate();

}

void end_cpu_data(void)
{
	system_level.reset_pstate_data();
	
	perf_events->clear();
}
