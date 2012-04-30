/*
 * Copyright 2010, Intel Corporation
 *
 * This is part of PowerTOP
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
 * getopt code is taken from "The GNU C Library" reference manual,
 * section 24.2 "Parsing program options using getopt"
 * http://www.gnu.org/s/libc/manual/html_node/Getopt-Long-Option-Example.html
 * Manual published under the terms of the Free Documentation License.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <getopt.h>
#include <unistd.h>
#include <locale.h>

#include "cpu/cpu.h"
#include "process/process.h"
#include "perf/perf.h"
#include "perf/perf_bundle.h"
#include "lib.h"


#include "devices/device.h"
#include "devices/usb.h"
#include "measurement/measurement.h"
#include "parameters/parameters.h"
#include "calibrate/calibrate.h"


#include "tuning/tuning.h"

#include "display.h"
#include "devlist.h"
#include "report.h"

#define DEBUGFS_MAGIC          0x64626720

int debug_learning = 0;
unsigned time_out = 20;
int leave_powertop = 0;

static const struct option long_options[] =
{
	/* These options set a flag. */
	{"debug", no_argument, &debug_learning, 'd'},
	{"version", no_argument, NULL, 'V'},
	{"help",no_argument, NULL, 'u'}, /* u for usage */
	{"calibrate",no_argument, NULL, 'c'},
	{"html", optional_argument, NULL, 'h'},
	{"csv", optional_argument, NULL, 'C'},
	{"extech", optional_argument, NULL, 'e'},
	{"time", optional_argument, NULL, 't'},
	{"iteration", optional_argument, NULL, 'i'},
	{NULL, 0, NULL, 0}
};

static void print_version()
{
	printf(_("Powertop version" POWERTOP_VERSION ", compiled on "__DATE__ "\n"));
}

static bool set_refresh_timeout()
{
	static char buf[4];
	mvprintw(1, 0, "%s (currently %u): ", _("Set refresh time out"), time_out);
	memset(buf, '\0', sizeof(buf));
	get_user_input(buf, sizeof(buf) - 1);
	show_tab(0);
	unsigned time = strtoul(buf, NULL, 0);
	if (!time) return 0;
	if (time > 32) time = 32;
	time_out = time;
	return 1;
}

static void print_usage()
{
	printf(_("Usage: powertop [OPTIONS]\n\n"));
	printf(_("--debug \t\t run in \"debug\" mode\n"));
	printf(_("--version \t\t print version information\n"));
	printf(_("--calibrate \t\t runs powertop in calibration mode\n"));
	printf(_("--extech=devnode \t uses an Extech Power Analyzer for measurements\n"));
	printf(_("--html[=FILENAME] \t generate a html report\n"));
	printf(_("--csv[=FILENAME] \t generate a csv report\n"));
	printf(_("--time[=secs] \t\t generate a report for 'x' secs\n"));
	printf(_("--iteration[=iterations] number of times to run each test\n"));
	printf(_("--help \t\t\t print this help menu\n"));
	printf("\n");
	printf(_("For more help please refer to the README\n\n"));
}

static void do_sleep(int seconds)
{
	time_t target;
	int delta;

	if (!ncurses_initialized()) {
		sleep(seconds);
		return;
	}
#ifndef DISABLE_NCURSES
	target = time(NULL) + seconds;
	delta = seconds;
	do {
		int c;
		usleep(6000);
		halfdelay(delta * 10);

		c = getch();

		switch (c) {
		case KEY_NPAGE:
		case KEY_RIGHT:
			show_next_tab();
			break;
		case KEY_PPAGE:
		case KEY_LEFT:
			show_prev_tab();
			break;
		case KEY_DOWN:
			cursor_down();
			break;
		case KEY_UP:
			cursor_up();
			break;
		case 10:
			cursor_enter();
			break;
		case 's':
			if (set_refresh_timeout())
				return;
			break;
		case 'r':
			window_refresh();
			return;
		case KEY_EXIT:
		case 'q':
		case 27:
			leave_powertop = 1;
			return;
		}

		delta = target - time(NULL);
		if (delta <= 0)
			break;

	} while (1);
#endif
}


void one_measurement(int seconds)
{
	create_all_usb_devices();
	start_power_measurement();
	devices_start_measurement();
	start_process_measurement();
	start_cpu_measurement();

	do_sleep(seconds);

	end_cpu_measurement();
	end_process_measurement();
	collect_open_devices();
	devices_end_measurement();
	end_power_measurement();

	process_cpu_data();
	process_process_data();

	/* output stats */
	process_update_display();
	report_summary();
	w_display_cpu_cstates();
	w_display_cpu_pstates();
	report_display_cpu_cstates();
	report_display_cpu_pstates();
	report_process_update_display();

	tuning_update_display();

	end_process_data();

	global_joules_consumed();
	compute_bundle();

	show_report_devices();
	report_show_open_devices();

	report_devices();

	store_results(measurement_time);
	end_cpu_data();
}

void out_of_memory()
{
	reset_display();
	printf("Out of memory. Aborting...\n");
	abort();
}

static void load_board_params()
{
	string boardname;
	char filename[4096];

	boardname = read_sysfs_string("/etc/boardname");

	if (boardname.length() < 2)
		return;

	sprintf(filename, "/var/cache/powertop/saved_parameters.powertop.%s", boardname.c_str());

	if (access(filename, R_OK))
		return;

	load_parameters(filename);
	global_fixed_parameters = 1;
	global_power_override = 1;
}

void report(int time, int iterations, char *file)
{

	/* one to warm up everything */
	fprintf(stderr, _("Preparing to take measurements\n"));
	utf_ok = 0;
	one_measurement(1);
	fprintf(stderr, _("Measuring %d time(s) for %d seconds each\n"),iterations,time);
	for (int i=0; i != iterations; i++){
		init_report_output(file);
		initialize_tuning();
		/* and then the real measurement */
		one_measurement(time);
		report_show_tunables();
		finish_report_output();
		clear_tuning();
	}
	/* and wrap up */
	learn_parameters(50, 0);
	save_all_results("saved_results.powertop");
	save_parameters("saved_parameters.powertop");
	end_pci_access();
	exit(0);
}

static void checkroot() {
	int uid; 
	uid = getuid();

	if (uid != 0) {
		printf(_("PowerTOP " POWERTOP_VERSION " must be run with root privileges.\n"));
		printf(_("exiting...\n"));
		exit(EXIT_FAILURE);
	}
	
}
int main(int argc, char **argv)
{
	int ret;
	int option_index;
	int c;
	bool wantreport = FALSE;
	char filename[4096];
	int  iterations = 1;
	struct statfs st_fs;

#ifndef DISABLE_TRYCATCH
	set_new_handler(out_of_memory);
#endif

	setlocale (LC_ALL, "");
#ifndef DISABLE_I18N
	bindtextdomain ("powertop", "/usr/share/locale");
	textdomain ("powertop");
#endif

	while (1) { /* parse commandline options */
		c = getopt_long (argc, argv, "ch:C:i:t:uV", long_options, &option_index);
		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
			case 'V':
				print_version();
				exit(0);
				break;

			case 'e': /* Extech power analyzer support */
				checkroot(); 
				extech_power_meter(optarg ? optarg : "/dev/ttyUSB0");
				break;
			case 'u':
				print_usage();
				exit(0);
				break;

			case 'c':
				calibrate();
				break;

			case 'h': /* html report */
				wantreport = TRUE;
				reporttype = 1;
				sprintf(filename, "%s", optarg ? optarg : "powertop.html" );
				break;

			case 't':
				time_out = (optarg ? atoi(optarg) : 20);
				break;

			case 'i':
				iterations = (optarg ? atoi(optarg) : 1);
				break;

			case 'C': /* csv report*/
				wantreport = TRUE;
				reporttype = 0;
				sprintf(filename, "%s", optarg ? optarg : "powertop.csv");
				break;
			case '?': /* Unknown option */
				/* getopt_long already printed an error message. */
				exit(0);
				break;
		}
	}
	
	checkroot(); 
	ret = system("/sbin/modprobe cpufreq_stats > /dev/null 2>&1");
	ret = system("/sbin/modprobe msr > /dev/null 2>&1");

	statfs("/sys/kernel/debug", &st_fs);
	if (st_fs.f_type != (long) DEBUGFS_MAGIC) {
		if (access("/bin/mount", X_OK) == 0) {
			ret = system("/bin/mount -t debugfs debugfs /sys/kernel/debug > /dev/null 2>&1");
		} else {
			ret = system("mount -t debugfs debugfs /sys/kernel/debug > /dev/null 2>&1");
		}
		if (ret != 0) {
			printf(_("Failed to mount debugfs!\n"));
			printf(_("exiting...\n"));
			exit(EXIT_FAILURE);
		}
	}

	srand(time(NULL));

	if (access("/var/cache/", W_OK) == 0)
		mkdir("/var/cache/powertop", 0600);
	else
		mkdir("/data/local/powertop", 0600);

	load_results("saved_results.powertop");
	load_parameters("saved_parameters.powertop");

	enumerate_cpus();
	create_all_devices();
	detect_power_meters();

	register_parameter("base power", 100, 0.5);
	register_parameter("cpu-wakeups", 39.5);
	register_parameter("cpu-consumption", 1.56);
	register_parameter("gpu-operations", 0.5576);
	register_parameter("disk-operations-hard", 0.2);
	register_parameter("disk-operations", 0.0);
	register_parameter("xwakes", 0.1);

	load_board_params();

	if (wantreport)
		 report(time_out, iterations, filename);

	if (debug_learning)
		printf("Learning debugging enabled\n");



	learn_parameters(250, 0);
	save_parameters("saved_parameters.powertop");


	if (debug_learning) {
	        learn_parameters(1000, 1);
		dump_parameter_bundle();
		end_pci_access();
		exit(0);
	}


	/* first one is short to not let the user wait too long */
	init_display();
	one_measurement(1);
	initialize_tuning();
	tuning_update_display();
	show_tab(0);



	while (!leave_powertop) {
		one_measurement(time_out);
		show_cur_tab();
		learn_parameters(15, 0);
	}
#ifndef DISABLE_NCURSES
	endwin();
#endif
	printf(_("Leaving PowerTOP\n"));


	end_process_data();
	clear_process_data();
	end_cpu_data();
	clear_cpu_data();

	save_all_results("saved_results.powertop");
	save_parameters("saved_parameters.powertop");
	learn_parameters(500, 0);
	save_parameters("saved_parameters.powertop");
	end_pci_access();
	clear_tuning();
	reset_display();

	clear_all_devices();
	clear_all_cpus();

	return 0;


}
