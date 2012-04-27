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
#include "../report.h"

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
	char packagename[128];
	if (strcmp(vendor, "GenuineIntel") == 0) {
		if (family == 6)
			switch (model) {
			case 0x1A:	/* Core i7, Xeon 5500 series */
			case 0x1E:	/* Core i7 and i5 Processor - Lynnfield Jasper Forest */
			case 0x1F:	/* Core i7 and i5 Processor - Nehalem */
			case 0x2E:	/* Nehalem-EX Xeon */
			case 0x2F:	/* Westmere-EX Xeon */
			case 0x25:	/* Westmere */
			case 0x27:	/* Medfield Atom*/
			case 0x2C:	/* Westmere */
				ret = new class nhm_package;
				break;
			case 0x2A:	/* SNB */
			case 0x2D:	/* SNB Xeon */
			case 0x3A:      /* IVB */
			case 0x3D:      /* IVB Xeon */
				has_c2c7_res = 1;
				ret = new class nhm_package;
				break;
			}
	}

	if (!ret)
		ret = new class cpu_package;

	ret->set_number(package, cpu);
	ret->childcount = 0;

	sprintf(packagename, _("cpu package %i"), cpu);
	cpudev = new class cpudevice(_("cpu package"), packagename, ret);
	all_devices.push_back(cpudev);
	return ret;
}

static class abstract_cpu * new_core(int core, int cpu, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;

	if (strcmp(vendor, "GenuineIntel") == 0) {
		if (family == 6)
			switch (model) {
			case 0x1A:	/* Core i7, Xeon 5500 series */
			case 0x1E:	/* Core i7 and i5 Processor - Lynnfield Jasper Forest */
			case 0x1F:	/* Core i7 and i5 Processor - Nehalem */
			case 0x2E:	/* Nehalem-EX Xeon */
			case 0x2F:	/* Westmere-EX Xeon */
			case 0x25:	/* Westmere */
			case 0x27:	/* Medfield Atom*/
			case 0x2C:	/* Westmere */
			case 0x2A:	/* SNB */
			case 0x2D:	/* SNB Xeon */
			case 0x3A:      /* IVB */
			case 0x3D:      /* IVB Xeon */
				ret = new class nhm_core;
			}
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
		if (family == 6)
			switch (model) {
			case 0x1A:	/* Core i7, Xeon 5500 series */
			case 0x1E:	/* Core i7 and i5 Processor - Lynnfield Jasper Forest */
			case 0x1F:	/* Core i7 and i5 Processor - Nehalem */
			case 0x2E:	/* Nehalem-EX Xeon */
			case 0x2F:	/* Westmere-EX Xeon */
			case 0x25:	/* Westmere */
			case 0x27:	/* Medfield Atom*/
			case 0x2C:	/* Westmere */
			case 0x2A:	/* SNB */
			case 0x2D:	/* SNB Xeon */
			case 0x3A:      /* IVB */
			case 0x3D:      /* IVB Xeon */
				ret = new class nhm_cpu;
			}
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
		}
		if (strncasecmp(line, "bogomips\t", 9) == 0) {
			handle_one_cpu(number, vendor, family, model);
			set_max_cpu(number);
		}
	}


	file.close();

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


static const char *core_class(int line)
{
	if (line & 1)
		return "core_odd";
	return "core_even";
}

static const char *package_class(int line)
{
	if (line & 1)
		return "package_odd";
	return "package_even";
}

static const char *cpu_class(int line, int cpu)
{
	if (line & 1) {
		if (cpu & 1)
			return "cpu_odd_odd";
		return "cpu_odd_even";
	}
	if (cpu & 1)
		return "cpu_even_odd";
	return "cpu_even_even";
}

static const char *freq_class(int line)
{
	if (line & 1) {
		return "cpu_odd_freq";
	}
	return "cpu_even_req";
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

void report_display_cpu_cstates(void)
{
	char buffer[512], buffer2[512];
	unsigned int package, core, cpu;
	int line;
	class abstract_cpu *_package, * _core, * _cpu;

	if ((!reportout.csv_report)&&(!reportout.http_report))
		return;

	if (reporttype)
		fprintf(reportout.http_report,
			"<div id=\"cpuidle\"><h2>Processor Idle state report</h2>\n");
	else
		fprintf(reportout.csv_report,
			"**Processor Idle State Report**, \n");


	for (package = 0; package < system_level.children.size(); package++) {
		int first_pkg = 0;
		_package = system_level.children[package];
		if (!_package)
			continue;

		if (reporttype)
			fprintf(reportout.http_report,"<table width=\"100%%\">\n");
		else
			fprintf(reportout.csv_report,"\n");

		for (core = 0; core < _package->children.size(); core++) {
			int actual_line = 0;
			_core = _package->children[core];
			if (!_core)
				continue;

			for (line = LEVEL_HEADER; line < 10; line++) {
				int first = 1;
				if (!_package->has_cstate_level(line))
					continue;

				actual_line++;
				if (reporttype)
					fprintf(reportout.http_report, "<tr>");

				buffer[0] = 0;
				buffer2[0] = 0;

				if (line == LEVEL_HEADER) {
					if (first_pkg == 0) {
						if (reporttype)
							fprintf(reportout.http_report,
										"<th colspan=2 class=\"package_header\" width=\"25%%\">%s</th>"
										, _package->fill_cstate_line(line, buffer));
						else
							fprintf(reportout.csv_report, ",%s,", "Package");

					} else {
						if (reporttype)
							fprintf(reportout.http_report, "<th colspan=2 class=\"package_header\">&nbsp;</th>");
					}

				} else if (first_pkg == 0) {

					if (reporttype)
						fprintf(reportout.http_report, "<td class=\"%s\" width=\"10%%\">%s</td><td class=\"%s\">%s</td>",
							freq_class(actual_line),
							_package->fill_cstate_name(line, buffer),
							package_class(actual_line),
							_package->fill_cstate_line(line, buffer2));
					else
						fprintf(reportout.csv_report, "%s, %s,",
							_package->fill_cstate_name(line, buffer),
							_package->fill_cstate_line(line, buffer2));


				} else {
					if (reporttype)
						fprintf(reportout.http_report,"<td colspan=2>&nbsp;</td>");
				}


				if (reporttype)
						fprintf(reportout.http_report,"<td width=\"2%%\">&nbsp;</td>");

				if (!_core->can_collapse()) {
					buffer[0] = 0;
					buffer2[0] = 0;

					if (line == LEVEL_HEADER) {
						if (reporttype) {
							fprintf(reportout.http_report,
								"<th colspan=2 class=\"core_header\" width=\"25%%\">%s</th>",
								_core->fill_cstate_line(line, buffer2));
					    } else {
							if (!first_pkg==0) {
								fprintf(reportout.csv_report,
									",,,Core %s ,,",
									_core->fill_cstate_line(line, buffer2));
							} else {
							fprintf(reportout.csv_report,
									",Core %s ,,",
									_core->fill_cstate_line(line, buffer2));
						}
					    }
					} else {
						if (reporttype) {
							fprintf(reportout.http_report,
								"<td class=\"%s\" width=\"10%%\">%s</td><td class=\"%s\">%s</td>",
								freq_class(actual_line),
								_core->fill_cstate_name(line, buffer),
								core_class(actual_line),
								_core->fill_cstate_line(line, buffer2));
						} else {
							if (first_pkg==0) {
								fprintf(reportout.csv_report,
									"%s, %s,",
									_core->fill_cstate_name(line, buffer),
									_core->fill_cstate_line(line, buffer2));
							} else {
								fprintf(reportout.csv_report,
									",,%s, %s,",
									_core->fill_cstate_name(line, buffer),
									_core->fill_cstate_line(line, buffer2));
							}
						}
					}
				}

				if (reporttype)
					fprintf(reportout.http_report,"<td width=\"2%%\">&nbsp;</td>");

				for (cpu = 0; cpu < _core->children.size(); cpu++) {
					_cpu = _core->children[cpu];
					if (!_cpu)
						continue;

					if (line == LEVEL_HEADER) {
						if (reporttype)
							fprintf(reportout.http_report,"<th colspan=3 class=\"cpu_header\">%s</th>",
								_cpu->fill_cstate_line(line, buffer));
						else
							fprintf(reportout.csv_report, ",%s,",
									_cpu->fill_cstate_line(line, buffer));
					} else {
						if (first == 1) {
							if (reporttype )
								fprintf(reportout.http_report,"<td class=\"%s\" width=\"10%%\">%s</td>",
									freq_class(actual_line),
									_cpu->fill_cstate_name(line, buffer));
							else
								fprintf(reportout.csv_report, "%s,",
									_cpu->fill_cstate_name(line, buffer));

							first = 0;
							buffer[0] = 0;

							if (reporttype) {
								sprintf(buffer2, "</td><td class=\"%s\">",
									cpu_class(actual_line, cpu));

								fprintf(reportout.http_report,"<td class=\"%s\">%s</td>",
									cpu_class(actual_line, cpu),
									_cpu->fill_cstate_line(line, buffer, buffer2));
							}else {
								fprintf(reportout.csv_report,"%s,",
									_cpu->fill_cstate_line(line, buffer, ","));
							}
						} else {
							buffer[0] = 0;

							if (reporttype) {
								sprintf(buffer2, "</td><td class=\"%s\">",
									cpu_class(actual_line, cpu));

								fprintf(reportout.http_report,"<td colspan=2 class=\"s\">%s</td>",
									_cpu->fill_cstate_line(line, buffer, buffer2));
							} else {
								fprintf(reportout.csv_report, "%s,",
									_cpu->fill_cstate_line(line, buffer, ","));
							}
						}
					}

				}
				if (reporttype)
					fprintf(reportout.http_report, "</tr>\n");
				else
					fprintf(reportout.csv_report, "\n");
			}

			first_pkg++;
		}
		if (reporttype)
			fprintf(reportout.http_report,"</table></div>\n");
		else
			fprintf(reportout.csv_report, "\n");
	}
}

void report_display_cpu_pstates(void)
{
	char buffer[512], buffer2[512];
	unsigned int package, core, cpu;
	int line;
	class abstract_cpu *_package, * _core, * _cpu;

	if ((!reportout.csv_report)&&(!reportout.http_report))
		return;

	if (reporttype)
		fprintf(reportout.http_report, "<div id=\"cpufreq\"><h2>Processor frequency report</h2>\n");
	else
		fprintf(reportout.csv_report, "**Processor Frequency Report**, \n");

	for (package = 0; package < system_level.children.size(); package++) {
		int first_pkg = 0;
		_package = system_level.children[package];
		if (!_package)
			continue;

		if (reporttype)
			fprintf(reportout.http_report,"<table width=\"100%%\">\n");
		else
			fprintf(reportout.csv_report,"\n");


		for (core = 0; core < _package->children.size(); core++) {
			_core = _package->children[core];
			if (!_core)
				continue;

			for (line = LEVEL_HEADER; line < 10; line++) {
				int first = 1;

				if (!_package->has_pstate_level(line))
					continue;


				if (reporttype)
					fprintf(reportout.http_report,"<tr>");

				buffer[0] = 0;
				buffer2[0] = 0;
				if (first_pkg == 0) {
					if (line == LEVEL_HEADER) {
						 if (reporttype)
								fprintf(reportout.http_report,
									"<th colspan=2 class=\"package_header\" width=\"25%%\">%s%s</th>",
									_package->fill_pstate_name(line, buffer),
									_package->fill_pstate_line(line, buffer2));
						else
								fprintf(reportout.csv_report,",%s,", "Package");

					} else {
						if (reporttype) {
							fprintf(reportout.http_report,
								"<td class=\"%s\" width=\"10%%\">%s</td><td class=\"%s\">%s</td>",
								freq_class(line),
								_package->fill_pstate_name(line, buffer),
								package_class(line),
								_package->fill_pstate_line(line, buffer2));
						} else {
							fprintf(reportout.csv_report,"%s, %s,",
								_package->fill_pstate_name(line, buffer),
								_package->fill_pstate_line(line, buffer2));
						}
					}
				} else {
					if (reporttype)
						fprintf(reportout.http_report,"<td colspan=2>&nbsp;</td>");
				}


				if (reporttype)
					fprintf(reportout.http_report,"<td width=\"2%%\">&nbsp;</td>");

				if (!_core->can_collapse()) {
					buffer[0] = 0;
					buffer2[0] = 0;
					if (line == LEVEL_HEADER) {
						if (reporttype) {
						fprintf(reportout.http_report,
							"<th colspan=2 class=\"core_header\" width=\"25%%\">%s%s</th>",
							_core->fill_pstate_name(line, buffer),
							_core->fill_pstate_line(line, buffer2));
						} else {
							if (first_pkg == 0) {
								fprintf(reportout.csv_report, "%s,%s,,",
									_core->fill_pstate_name(line, buffer),
									_core->fill_pstate_line(line, buffer2));
							} else {
								fprintf(reportout.csv_report, ",,%s,%s,,",
									_core->fill_pstate_name(line, buffer),
									_core->fill_pstate_line(line, buffer2));

							}
						}
					}else {
						if (reporttype)	{
							fprintf(reportout.http_report,
								"<td class=\"%s\" width=\"10%%\">%s</td><td class=\"%s\">%s</td>",
								freq_class(line),
								_core->fill_pstate_name(line, buffer),
								core_class(line),
								_core->fill_pstate_line(line, buffer2));
						} else {
							if (first_pkg == 0) {
								fprintf(reportout.csv_report, "%s,%s,",
									_core->fill_pstate_name(line, buffer),
									_core->fill_pstate_line(line, buffer2));
							} else {
								fprintf(reportout.csv_report, ",,%s,%s,",
									_core->fill_pstate_name(line, buffer),
									_core->fill_pstate_line(line, buffer2));
							}
						}
					}
				}

				if (reporttype)
					fprintf(reportout.http_report,"<td width=\"2%%\">&nbsp;</td>");

				for (cpu = 0; cpu < _core->children.size(); cpu++) {
					buffer[0] = 0;
					_cpu = _core->children[cpu];
					if (!_cpu)
						continue;

					if (line == LEVEL_HEADER) {
						if (reporttype)
							fprintf(reportout.http_report,
								"<th colspan=2 class=\"cpu_header\">%s</th>",
								_cpu->fill_pstate_line(line, buffer));
						else
							fprintf(reportout.csv_report, "%s,",
								_cpu->fill_pstate_line(line, buffer));
					} else {
						if (first == 1) {
							if (reporttype)
								fprintf(reportout.http_report,
									"<td class=\"%s\" width=\"10%%\">%s</td>",
									freq_class(line),
									_cpu->fill_pstate_name(line, buffer));
							else
								fprintf(reportout.csv_report,"%s,",
									_cpu->fill_pstate_name(line, buffer));

							first = 0;
							buffer[0] = 0;

							if (reporttype)
								fprintf(reportout.http_report,"<td class=\"%s\">%s</td>",
									cpu_class(line, cpu),
									_cpu->fill_pstate_line(line, buffer));
							else
								fprintf(reportout.csv_report, "%s,",
									_cpu->fill_pstate_line(line, buffer));

						} else {

							buffer[0] = 0;

							if (reporttype)
								fprintf(reportout.http_report,
									"<td colspan=2 class=\"%s\">%s</td>",
									cpu_class(line, cpu),
									_cpu->fill_pstate_line(line, buffer));
							else
								fprintf(reportout.csv_report,"%s,",
									_cpu->fill_pstate_line(line, buffer));

						}
					}
				}
				if (reporttype)
					fprintf(reportout.http_report,"</tr>\n");
				else
					fprintf(reportout.csv_report, "\n");

			}

			first_pkg++;
		}


	}

	if (reporttype)
		fprintf(reportout.http_report,"</table></div>");
	else
		fprintf(reportout.csv_report, "\n");
}

void impl_w_display_cpu_states(int state)
{
#ifndef DISABLE_NCURSES
	WINDOW *win;
	char buffer[128];
	char linebuf[1024];
	unsigned int package, core, cpu;
	int line;
	class abstract_cpu *_package, * _core, * _cpu;
	int ctr = 0;

	if (state == PSTATE)
		win = get_ncurses_win("Frequency stats");
	else
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
#endif // DISABLE_NCURSES
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
        struct record rec; /* holder */
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
                        fprintf(stderr, "cpu_idle event returned no state?\n");
                        exit(-1);
                }

		if (val == 4294967295)
			cpu->go_unidle(time);
		else
			cpu->go_idle(time);
	}

	if (strcmp(event->name, "power_frequency") == 0 
	|| strcmp(event->name, "cpu_frequency") == 0){

		ret = pevent_get_field_val(NULL, event, "state", &rec, &val, 0);
		if (ret < 0) {
			fprintf(stderr, "power or cpu_frequecny event returned no state?\n");
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
