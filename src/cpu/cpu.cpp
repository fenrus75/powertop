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
#include <unistd.h>
#include "cpu.h"
#include "cpudevice.h"
#include "cpu_rapl_device.h"
#include "dram_rapl_device.h"
#include "intel_cpus.h"
#include "../parameters/parameters.h"

#include "../perf/perf_bundle.h"
#include "../lib.h"
#include "../display.h"
#include "../report/report.h"
#include "../report/report-maker.h"

static class abstract_cpu system_level;

vector<class abstract_cpu *> all_cpus;

static	class perf_bundle * perf_events;



class perf_power_bundle: public perf_bundle
{
	virtual void handle_trace_point(void *trace, int cpu, uint64_t time);

};


static class abstract_cpu * new_package(int package, int cpu, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;
	class cpudevice *cpudev;
	class cpu_rapl_device *cpu_rapl_dev;
	class dram_rapl_device *dram_rapl_dev;

	char packagename[128];
	if (strcmp(vendor, "GenuineIntel") == 0)
		if (family == 6)
			if (is_supported_intel_cpu(model))
				ret = new class nhm_package(model);

	if (!ret)
		ret = new class cpu_package;

	ret->set_number(package, cpu);
	ret->set_type("Package");
	ret->childcount = 0;

	sprintf(packagename, _("cpu package %i"), cpu);
	cpudev = new class cpudevice(_("cpu package"), packagename, ret);
	all_devices.push_back(cpudev);

	sprintf(packagename, _("cpu rapl package %i"), cpu);
	cpu_rapl_dev = new class cpu_rapl_device(cpudev, _("cpu rapl package"), packagename, ret);
	if (cpu_rapl_dev->device_present())
		all_devices.push_back(cpu_rapl_dev);

	sprintf(packagename, _("dram rapl package %i"), cpu);
	dram_rapl_dev = new class dram_rapl_device(cpudev, _("dram rapl package"), packagename, ret);
	if (dram_rapl_dev->device_present())
		all_devices.push_back(dram_rapl_dev);

	return ret;
}

static class abstract_cpu * new_core(int core, int cpu, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;

	if (strcmp(vendor, "GenuineIntel") == 0)
		if (family == 6)
			if (is_supported_intel_cpu(model))
				ret = new class nhm_core(model);

	if (!ret)
		ret = new class cpu_core;
	ret->set_number(core, cpu);
	ret->childcount = 0;
	ret->set_type("Core");

	return ret;
}

static class abstract_cpu * new_i965_gpu(void)
{
	class abstract_cpu *ret = NULL;

	ret = new class i965_core;
	ret->childcount = 0;
	ret->set_type("GPU");

	return ret;
}

static class abstract_cpu * new_cpu(int number, char * vendor, int family, int model)
{
	class abstract_cpu * ret = NULL;

	if (strcmp(vendor, "GenuineIntel") == 0)
		if (family == 6)
			if (is_supported_intel_cpu(model))
				ret = new class nhm_cpu;

	if (!ret)
		ret = new class cpu_linux;
	ret->set_number(number, number);
	ret->set_type("CPU");
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
		if (package_number == (unsigned int) -1)
			package_number = 0;
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

static void handle_i965_gpu(void)
{
	unsigned int core_number = 0;
	class abstract_cpu *package;


	package = system_level.children[0];

	core_number = package->children.size();

	if (package->children.size() <= core_number)
		package->children.resize(core_number + 1, NULL);

	if (!package->children[core_number]) {
		package->children[core_number] = new_i965_gpu();
		package->childcount++;
	}
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
	/* Not all /proc/cpuinfo include "vendor_id\t". */
	vendor[0] = '\0';

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
		if (strncasecmp(line, "bogomips\t", 9) == 0) {
			if (number == -1) {
				/* Not all /proc/cpuinfo include "processor\t". */
				number = 0;
			}
			if (number >= 0) {
				handle_one_cpu(number, vendor, family, model);
				set_max_cpu(number);
				number = -2;
			}
		}
	}


	file.close();

	if (access("/sys/class/drm/card0/power/rc6_residency_ms", R_OK) == 0)
		handle_i965_gpu();

	perf_events = new perf_power_bundle();

	if (!perf_events->add_event("power:cpu_idle")){
		perf_events->add_event("power:power_start");
		perf_events->add_event("power:power_end");
	}
	if (!perf_events->add_event("power:cpu_frequency"))
		perf_events->add_event("power:power_frequency");

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

static int has_state_level(class abstract_cpu *acpu, int state, int line)
{
	switch (state) {
		case PSTATE:
			return acpu->has_pstate_level(line);
			break;
		case CSTATE:
			return acpu->has_cstate_level(line);
			break;
	}
	return 0;
}

static const char * fill_state_name(class abstract_cpu *acpu, int state, int line, char *buf)
{
	switch (state) {
		case PSTATE:
			return acpu->fill_pstate_name(line, buf);
			break;
		case CSTATE:
			return acpu->fill_cstate_name(line, buf);
			break;
	}
	return "-EINVAL";
}

static const char * fill_state_line(class abstract_cpu *acpu, int state, int line,
					char *buf, const char *sep = "")
{
	switch (state) {
		case PSTATE:
			return acpu->fill_pstate_line(line, buf);
			break;
		case CSTATE:
			return acpu->fill_cstate_line(line, buf, sep);
			break;
	}
	return "-EINVAL";
}

static int get_cstates_num(void)
{
	unsigned int package, core, cpu;
	class abstract_cpu *_package, * _core, * _cpu;
	unsigned int i;
	int cstates_num;

	for (package = 0, cstates_num = 0;
			package < system_level.children.size(); package++) {
		_package = system_level.children[package];
		if (_package == NULL)
			continue;

		/* walk package cstates and get largest cstates number */
		for (i = 0; i < _package->cstates.size(); i++)
			cstates_num = std::max(cstates_num,
						(_package->cstates[i])->line_level);

		/*
		 * for each core in this package, walk core cstates and get
		 * largest cstates number
		 */
		for (core = 0; core < _package->children.size(); core++) {
			_core = _package->children[core];
			if (_core == NULL)
				continue;

			for (i = 0; i <  _core->cstates.size(); i++)
				cstates_num = std::max(cstates_num,
						(_core->cstates[i])->line_level);

			/*
			 * for each core, walk the logical cpus in case
			 * there is are more linux cstates than hw cstates
			 */
			 for (cpu = 0; cpu < _core->children.size(); cpu++) {
				_cpu = _core->children[cpu];
				if (_cpu == NULL)
					continue;

				for (i = 0; i < _cpu->cstates.size(); i++)
					cstates_num = std::max(cstates_num,
						(_cpu->cstates[i])->line_level);
			}
		}
	}

	return cstates_num;
}

void report_display_cpu_cstates(void)
{
	char buffer[512], buffer2[512];
	unsigned int package, core, cpu;
	int line, cstates_num;
	class abstract_cpu *_package, * _core, * _cpu;

	cstates_num = get_cstates_num();

	report.begin_section(SECTION_CPUIDLE);
	report.add_header("Processor Idle state report");

	for (package = 0; package < system_level.children.size(); package++) {
		bool first_core = true;

		_package = system_level.children[package];
		if (!_package)
			continue;

		report.begin_table(TABLE_WIDE);
		for (core = 0; core < _package->children.size(); core++) {
			_core = _package->children[core];
			if (!_core)
				continue;

			for (line = LEVEL_HEADER; line <= cstates_num; line++) {
				bool first_cpu = true;

				if (!_package->has_cstate_level(line))
					continue;

				report.begin_row();
				buffer[0] = 0;
				buffer2[0] = 0;

				if (line == LEVEL_HEADER) {
					if (first_core) {
						report.begin_cell(CELL_FIRST_PACKAGE_HEADER);
						report.addf(__("Package %i"), _package->get_number());
					} else {
						report.begin_cell(CELL_EMPTY_PACKAGE_HEADER);
						report.add_empty_cell();
					}
				} else if (first_core) {
					report.begin_cell(CELL_STATE_NAME);
					report.add(_package->fill_cstate_name(line, buffer));
					report.begin_cell(CELL_PACKAGE_STATE_VALUE);
					report.add(_package->fill_cstate_line(line, buffer2));
				} else {
					report.begin_cell(CELL_EMPTY_PACKAGE_STATE);
					report.add_empty_cell();
				}

				report.begin_cell(CELL_SEPARATOR);
				report.add_empty_cell();

				if (!_core->can_collapse()) {
					buffer[0] = 0;
					buffer2[0] = 0;

					if (line == LEVEL_HEADER) {
						report.begin_cell(CELL_CORE_HEADER);
						/* Here we need to check for which core type we
  						 * are using. Do not use the core type for the 
  						 * report.addf as it breaks an important macro use 
  						 * for translation decision making for the reports. 
  						   */
						const char* core_type = _core->get_type(); 
					        if (core_type != NULL) {	
							if (strcmp(core_type, "Core") == 0 ) {
								report.addf(__("Core %i"), _core->get_number());
							} else {
								report.addf(__("GPU %i"), _core->get_number());
							} 
						}
                                       } else {
                                                report.begin_cell(CELL_STATE_NAME);
                                                report.add(_core->fill_cstate_name(line, buffer));
						report.begin_cell(CELL_CORE_STATE_VALUE);
						report.add(_core->fill_cstate_line(line, buffer2));
					}
				}

				report.begin_cell(CELL_SEPARATOR);
				report.add_empty_cell();

				for (cpu = 0; cpu < _core->children.size(); cpu++) {
					_cpu = _core->children[cpu];
					if (!_cpu)
						continue;

					report.set_cpu_number(cpu);
					if (line == LEVEL_HEADER) {
						report.begin_cell(CELL_CPU_CSTATE_HEADER);
						report.addf(__("CPU %i"), _cpu->get_number());
						continue;
					}

					if (first_cpu) {
						report.begin_cell(CELL_STATE_NAME);
						report.add(_cpu->fill_cstate_name(line, buffer));
						first_cpu = false;
					}

					buffer[0] = 0;
					report.begin_cell(CELL_CPU_STATE_VALUE);
					report.add(_cpu->fill_cstate_percentage(line, buffer));
					report.begin_cell(CELL_CPU_STATE_VALUE);
					if (line != LEVEL_C0)
						report.add(_cpu->fill_cstate_time(line, buffer));
					else
						report.add_empty_cell();
				}
			}

			first_core = false;
		}
	}
}

void report_display_cpu_pstates(void)
{
	char buffer[512], buffer2[512];
	unsigned int package, core, cpu;
	int line;
	class abstract_cpu *_package, * _core, * _cpu;
	unsigned int i, pstates_num;

	for (i = 0, pstates_num = 0; i < all_cpus.size(); i++)
		if (all_cpus[i])
			pstates_num = std::max<unsigned int>(pstates_num,
								all_cpus[i]->pstates.size());

	report.begin_section(SECTION_CPUFREQ);
	report.add_header("Processor Frequency Report");

	for (package = 0; package < system_level.children.size(); package++) {
		bool first_core = true;

		_package = system_level.children[package];
		if (!_package)
			continue;

		report.begin_table(TABLE_WIDE);
		for (core = 0; core < _package->children.size(); core++) {
			_core = _package->children[core];
			if (!_core)
				continue;

			if (!_core->has_pstates())
				continue;

			for (line = LEVEL_HEADER; line < (int)pstates_num; line++) {
				bool first_cpu = true;

				if (!_package->has_pstate_level(line))
					continue;

				report.begin_row();

				buffer[0] = 0;
				buffer2[0] = 0;
				if (first_core) {
					if (line == LEVEL_HEADER) {
						report.begin_cell(CELL_FIRST_PACKAGE_HEADER);
						report.addf(__("Package %i"), _package->get_number());
					} else {
						report.begin_cell(CELL_STATE_NAME);
						report.add(_package->fill_pstate_name(line, buffer));
						report.begin_cell(CELL_PACKAGE_STATE_VALUE);
						report.add(_package->fill_pstate_line(line, buffer2));
					}
				} else {
					report.begin_cell(CELL_EMPTY_PACKAGE_STATE);
					report.add_empty_cell();
				}

				report.begin_cell(CELL_SEPARATOR);
				report.add_empty_cell();

				if (!_core->can_collapse()) {
					buffer[0] = 0;
					buffer2[0] = 0;
					if (line == LEVEL_HEADER) {
						report.begin_cell(CELL_CORE_HEADER);
						report.addf(__("Core %i"), _core->get_number());
					} else {
						report.begin_cell(CELL_STATE_NAME);
						report.add(_core->fill_pstate_name(line, buffer));
						report.begin_cell(CELL_PACKAGE_STATE_VALUE);
						report.add(_core->fill_pstate_line(line, buffer2));
					}
				}

				report.begin_cell(CELL_SEPARATOR);
				report.add_empty_cell();

				for (cpu = 0; cpu < _core->children.size(); cpu++) {
					buffer[0] = 0;
					_cpu = _core->children[cpu];
					if (!_cpu)
						continue;

					report.set_cpu_number(cpu);
					if (line == LEVEL_HEADER) {
						report.begin_cell(CELL_CPU_PSTATE_HEADER);
						report.addf(__("CPU %i"), _cpu->get_number());
						continue;
					}

					if (first_cpu) {
						report.begin_cell(CELL_STATE_NAME);
						report.add(_cpu->fill_pstate_name(line, buffer));
						first_cpu = false;
					}

					buffer[0] = 0;
					report.begin_cell(CELL_CPU_STATE_VALUE);
					report.add(_cpu->fill_pstate_line(line, buffer));
				}
			}

			first_core = false;
		}
	}
}

void impl_w_display_cpu_states(int state)
{
	WINDOW *win;
	char buffer[128];
	char linebuf[1024];
	unsigned int package, core, cpu;
	int line, loop, cstates_num, pstates_num;
	class abstract_cpu *_package, * _core, * _cpu;
	int ctr = 0;
	unsigned int i;

	cstates_num = get_cstates_num();

	for (i = 0, pstates_num = 0; i < all_cpus.size(); i++) {
		if (!all_cpus[i])
			continue;

		pstates_num = std::max<int>(pstates_num, all_cpus[i]->pstates.size());
	}

	if (state == PSTATE) {
		win = get_ncurses_win("Frequency stats");
		loop = pstates_num;
	} else {
		win = get_ncurses_win("Idle stats");
		loop = cstates_num;
	}

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
			if (!_core->has_pstates() && state == PSTATE)
				continue;

			for (line = LEVEL_HEADER; line <= loop; line++) {
				int first = 1;
				ctr = 0;
				linebuf[0] = 0;

				if (!has_state_level(_package, state, line))
					continue;

				buffer[0] = 0;
				if (first_pkg == 0) {
					strcat(linebuf, fill_state_name(_package, state, line, buffer));
					expand_string(linebuf, ctr + 10);
					strcat(linebuf, fill_state_line(_package, state, line, buffer));
				}
				ctr += 20;
				expand_string(linebuf, ctr);

				strcat(linebuf, "| ");
				ctr += strlen("| ");

				if (!_core->can_collapse()) {
					buffer[0] = 0;
					strcat(linebuf, fill_state_name(_core, state, line, buffer));
					expand_string(linebuf, ctr + 10);
					strcat(linebuf, fill_state_line(_core, state, line, buffer));
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
						strcat(linebuf, fill_state_name(_cpu, state, line, buffer));
						expand_string(linebuf, ctr + 10);
						first = 0;
						ctr += 12;
					}
					buffer[0] = 0;
					strcat(linebuf, fill_state_line(_cpu, state, line, buffer));
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

void w_display_cpu_pstates(void)
{
	impl_w_display_cpu_states(PSTATE);
}

void w_display_cpu_cstates(void)
{
	impl_w_display_cpu_states(CSTATE);
}

struct power_entry {
#ifndef __i386__
	int dummy;
#endif
	int64_t	type;
	int64_t	value;
} __attribute__((packed));


void perf_power_bundle::handle_trace_point(void *trace, int cpunr, uint64_t time)
{
	struct event_format *event;
        struct pevent_record rec; /* holder */
	class abstract_cpu *cpu;
	int type;

	rec.data = trace;

	type = pevent_data_type(perf_event::pevent, &rec);
	event = pevent_find_event(perf_event::pevent, type);

	if (!event)
		return;

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
	unsigned long long val;
	int ret;
	if (strcmp(event->name, "cpu_idle")==0) {

		ret = pevent_get_field_val(NULL, event, "state", &rec, &val, 0);
                if (ret < 0) {
                        fprintf(stderr, _("cpu_idle event returned no state?\n"));
                        exit(-1);
                }

		if (val == (unsigned int)-1)
			cpu->go_unidle(time);
		else
			cpu->go_idle(time);
	}

	if (strcmp(event->name, "power_frequency") == 0
	|| strcmp(event->name, "cpu_frequency") == 0){

		ret = pevent_get_field_val(NULL, event, "state", &rec, &val, 0);
		if (ret < 0) {
			fprintf(stderr, _("power or cpu_frequency event returned no state?\n"));
			exit(-1);
		}

		cpu->change_freq(time, val);
	}

	if (strcmp(event->name, "power_start")==0)
		cpu->go_idle(time);
	if (strcmp(event->name, "power_end")==0)
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

void clear_cpu_data(void)
{
	if (perf_events)
		perf_events->release();
	delete perf_events;
}


void clear_all_cpus(void)
{
	unsigned int i;
	for (i = 0; i < all_cpus.size(); i++) {
		delete all_cpus[i];
	}
	all_cpus.clear();
}
