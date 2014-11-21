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
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <getopt.h>
#include <unistd.h>
#include <locale.h>
#include <sys/resource.h>

#include "cpu/cpu.h"
#include "process/process.h"
#include "perf/perf.h"
#include "perf/perf_bundle.h"
#include "lib.h"
#include "../config.h"


#include "devices/device.h"
#include "devices/devfreq.h"
#include "devices/usb.h"
#include "devices/ahci.h"
#include "measurement/measurement.h"
#include "parameters/parameters.h"
#include "calibrate/calibrate.h"


#include "tuning/tuning.h"

#include "display.h"
#include "devlist.h"
#include "report/report.h"

#define DEBUGFS_MAGIC          0x64626720

#define NR_OPEN_DEF 1024 * 1024

int debug_learning = 0;
unsigned time_out = 20;
int leave_powertop = 0;
void (*ui_notify_user) (const char *frmt, ...);

enum {
	OPT_AUTO_TUNE = CHAR_MAX + 1,
	OPT_EXTECH,
	OPT_DEBUG
};

static const struct option long_options[] =
{
	/* These options set a flag. */
	{"auto-tune",	no_argument,		NULL,		 OPT_AUTO_TUNE},
	{"calibrate",	no_argument,		NULL,		 'c'},
	{"csv",		optional_argument,	NULL,		 'C'},
	{"debug",	no_argument,		&debug_learning, OPT_DEBUG},
	{"extech",	optional_argument,	NULL,		 OPT_EXTECH},
	{"html",	optional_argument,	NULL,		 'r'},
	{"iteration",	optional_argument,	NULL,		 'i'},
	{"quiet",	no_argument,		NULL,		 'q'},
	{"time",	optional_argument,	NULL,		 't'},
	{"workload",	optional_argument,	NULL,		 'w'},
	{"version",	no_argument,		NULL,		 'V'},
	{"help",	no_argument,		NULL,		 'h'},
	{NULL,		0,			NULL,		 0}
};

static void print_version()
{
	printf(_("PowerTOP version" POWERTOP_VERSION ", compiled on " __DATE__ "\n"));
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
	printf("%s\n\n", _("Usage: powertop [OPTIONS]"));
	printf("     --auto-tune\t %s\n", _("sets all tunable options to their GOOD setting"));
	printf(" -c, --calibrate\t %s\n", _("runs powertop in calibration mode"));
	printf(" -C, --csv%s\t %s\n", _("[=filename]"), _("generate a csv report"));
	printf("     --debug\t\t %s\n", _("run in \"debug\" mode"));
	printf("     --extech%s\t %s\n", _("[=devnode]"), _("uses an Extech Power Analyzer for measurements"));
	printf(" -r, --html%s\t %s\n", _("[=filename]"), _("generate a html report"));
	printf(" -i, --iteration%s\n", _("[=iterations] number of times to run each test"));
	printf(" -q, --quiet\t\t %s\n", _("suppress stderr output"));
	printf(" -t, --time%s\t %s\n", _("[=seconds]"), _("generate a report for 'x' seconds"));
	printf(" -w, --workload%s %s\n", _("[=workload]"), _("file to execute for workload"));
	printf(" -V, --version\t\t %s\n", _("print version information"));
	printf(" -h, --help\t\t %s\n", _("print this help menu"));
	printf("\n");
	printf("%s\n\n", _("For more help please refer to the 'man 8 powertop'"));
}

static void do_sleep(int seconds)
{
	time_t target;
	int delta;

	if (!ncurses_initialized()) {
		sleep(seconds);
		return;
	}
	target = time(NULL) + seconds;
	delta = seconds;
	do {
		int c;
		usleep(6000);
		halfdelay(delta * 10);

		c = getch();
		switch (c) {
		case KEY_BTAB:
			show_prev_tab();
			break;
		case '\t':
			show_next_tab(); 
			break;
		case KEY_RIGHT:
			cursor_right(); 
			break;
		case KEY_LEFT:
			cursor_left(); 
			break;
		case KEY_NPAGE:
		case KEY_DOWN:
			cursor_down();
			break;
		case KEY_PPAGE:
		case KEY_UP:
			cursor_up();
			break;
		case ' ':
		case '\n':
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
		case 27:	// Escape
			leave_powertop = 1;
			return;
		}

		delta = target - time(NULL);
		if (delta <= 0)
			break;

	} while (1);
}


void one_measurement(int seconds, char *workload)
{
	create_all_usb_devices();
	start_power_measurement();
	devices_start_measurement();
	start_devfreq_measurement();
	start_process_measurement();
	start_cpu_measurement();

	if (workload && workload[0]) {
		if (system(workload))
			fprintf(stderr, _("Unknown issue running workload!\n"));
	} else {
		do_sleep(seconds);
	}
	end_cpu_measurement();
	end_process_measurement();
	collect_open_devices();
	end_devfreq_measurement();
	devices_end_measurement();
	end_power_measurement();

	process_cpu_data();
	process_process_data();

	/* output stats */
	process_update_display();
	report_summary();
	w_display_cpu_cstates();
	w_display_cpu_pstates();
	if (reporttype != REPORT_OFF) {
		report_display_cpu_cstates();
		report_display_cpu_pstates();
	}
	report_process_update_display();
	tuning_update_display();

	end_process_data();

	global_joules_consumed();
	compute_bundle();

	show_report_devices();
	report_show_open_devices();

	report_devices();
	display_devfreq_devices();
	report_devfreq_devices();
	ahci_create_device_stats_table();
	store_results(measurement_time);
	end_cpu_data();
}

void out_of_memory()
{
	reset_display();
	printf("%s...\n",_("PowerTOP is out of memory. PowerTOP is Aborting"));
	abort();
}

void make_report(int time, char *workload, int iterations, char *file)
{

	/* one to warm up everything */
	fprintf(stderr, _("Preparing to take measurements\n"));
	utf_ok = 0;
	one_measurement(1, NULL);

	if (!workload[0])
	  fprintf(stderr, _("Taking %d measurement(s) for a duration of %d second(s) each.\n"),iterations,time);
	else
	   fprintf(stderr, _("Measuring workload %s.\n"), workload);
	for (int i=0; i != iterations; i++){
		init_report_output(file, iterations);
		initialize_tuning();
		/* and then the real measurement */
		one_measurement(time, workload);
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

static int get_nr_open(void) {
	int nr_open = NR_OPEN_DEF;
	ifstream file;

	file.open("/proc/sys/fs/nr_open", ios::in);
	if (file) {
		file >> nr_open;
		file.close();
	}
	return nr_open;
}

static void powertop_init(void)
{
	static char initialized = 0;
	int ret;
	struct statfs st_fs;
	struct rlimit rlmt;

	if (initialized)
		return;

	checkroot();

	rlmt.rlim_cur = rlmt.rlim_max = get_nr_open();
	setrlimit (RLIMIT_NOFILE, &rlmt);

	if (system("/sbin/modprobe cpufreq_stats > /dev/null 2>&1"))
		fprintf(stderr, _("modprobe cpufreq_stats failed"));
	if (system("/sbin/modprobe msr > /dev/null 2>&1"))
		fprintf(stderr, _("modprobe msr failed"));
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
	create_all_devfreq_devices();
	detect_power_meters();

	register_parameter("base power", 100, 0.5);
	register_parameter("cpu-wakeups", 39.5);
	register_parameter("cpu-consumption", 1.56);
	register_parameter("gpu-operations", 0.5576);
	register_parameter("disk-operations-hard", 0.2);
	register_parameter("disk-operations", 0.0);
	register_parameter("xwakes", 0.1);

        load_parameters("saved_parameters.powertop");

	initialized = 1;
}

void clean_shutdown()
{
	close_results();
	clean_open_devices();
	clear_all_devices();
	clear_all_devfreq();
	clear_all_cpus();

	return;
}


int main(int argc, char **argv)
{
	int option_index;
	int c;
	char filename[4096];
	char workload[4096] = {0,};
	int  iterations = 1, auto_tune = 0;

	set_new_handler(out_of_memory);

	setlocale (LC_ALL, "");

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif
	ui_notify_user = ui_notify_user_ncurses;
	while (1) { /* parse commandline options */
		c = getopt_long(argc, argv, "cC:r:i:qt:w:Vh", long_options, &option_index);
		/* Detect the end of the options. */
		if (c == -1)
			break;
		switch (c) {
		case OPT_AUTO_TUNE:
			auto_tune = 1;
			leave_powertop = 1;
			ui_notify_user = ui_notify_user_console;
			break;
		case 'c':
			powertop_init();
			calibrate();
			break;
		case 'C':		/* csv report */
			reporttype = REPORT_CSV;
			sprintf(filename, "%s", optarg ? optarg : "powertop.csv");
			break;
		case OPT_DEBUG:
			/* implemented using getopt_long(3) flag */
			break;
		case OPT_EXTECH:	/* Extech power analyzer support */
			checkroot();
			extech_power_meter(optarg ? optarg : "/dev/ttyUSB0");
			break;
		case 'r':		/* html report */
			reporttype = REPORT_HTML;
			sprintf(filename, "%s", optarg ? optarg : "powertop.html");
			break;
		case 'i':
			iterations = (optarg ? atoi(optarg) : 1);
			break;
		case 'q':
			if (freopen("/dev/null", "a", stderr))
				fprintf(stderr, _("Quite mode failed!\n"));
			break;
		case 't':
			time_out = (optarg ? atoi(optarg) : 20);
			break;
		case 'w':		/* measure workload */
			sprintf(workload, "%s", optarg ? optarg : '\0');
			break;
		case 'V':
			print_version();
			exit(0);
			break;
		case 'h':
			print_usage();
			exit(0);
			break;
		case '?':		/* Unknown option */
			/* getopt_long already printed an error message. */
			exit(1);
			break;
		}
	}

	powertop_init();

	if (reporttype != REPORT_OFF)
		make_report(time_out, workload, iterations, filename);

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
	if (!auto_tune)
		init_display();

	initialize_devfreq();
	initialize_tuning();
	/* first one is short to not let the user wait too long */
	one_measurement(1, NULL);

	if (!auto_tune) {
		tuning_update_display();
		show_tab(0);
	} else {
		auto_toggle_tuning();
	}

	while (!leave_powertop) {
		if (!auto_tune)
			show_cur_tab();
		one_measurement(time_out, NULL);
		learn_parameters(15, 0);
	}
	if (!auto_tune)
		endwin();
	printf("%s\n", _("Leaving PowerTOP"));

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

	clean_shutdown();

	return 0;
}
