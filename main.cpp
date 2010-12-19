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
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "cpu/cpu.h"
#include "process/process.h"
#include "perf/perf.h"
#include "perf/perf_bundle.h"
#include "lib.h"
#include "html.h"

#include "devices/device.h"
#include "devices/usb.h"
#include "measurement/measurement.h"
#include "parameters/parameters.h"
#include "calibrate/calibrate.h"


#include "tuning/tuning.h"

#include "display.h"

int debug_learning;

int leave_powertop = 0;

static void do_sleep(int seconds)
{
	time_t target;
	int delta;

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
	devices_end_measurement();
	end_power_measurement();

	process_cpu_data();
	process_process_data();

	/* output stats */
	process_update_display();
	w_display_cpu_cstates();
	w_display_cpu_pstates();
	html_display_cpu_cstates();
	html_display_cpu_pstates();
	tuning_update_display();

	end_process_data();
		

	global_joules_consumed();
	compute_bundle();

	report_devices();
	html_report_devices();

	store_results(measurement_time);
	end_cpu_data();
}

int main(int argc, char **argv)
{
	system("/sbin/modprobe cpufreq_stats > /dev/null 2>&1");
	system("/bin/mount -t debugfs debugfs /sys/kernel/debug > /dev/null 2>&1");

	srand(time(NULL));


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

	if (argc > 1) {
		if (strcmp(argv[1], "--calibrate") == 0)
			calibrate();
	}

	if (argc > 1) {
		if (strcmp(argv[1], "--debug") == 0)
			debug_learning = 1;
	}

	if (argc > 2) {
		if (strcmp(argv[2], "--debug") == 0)
			debug_learning = 1;
	}

	if (argc > 1) {
		if (strcmp(argv[1], "--html") == 0) {
			init_html_output("powertop.html");
			fprintf(stderr, "Measuring for 20 seconds\n");
			one_measurement(1);
			initialize_tuning();

			finish_html_output();

			/* and wrap up */
			learn_parameters(50, 0);
			save_all_results("saved_results.powertop");
			save_parameters("saved_parameters.powertop");
			exit(0);
		}
	}

	if (debug_learning)
		printf("Learning debugging enabled\n");



        learn_parameters(250, 0);
	save_parameters("saved_parameters.powertop");


	if (debug_learning) {
	        learn_parameters(1000, 1);
		dump_parameter_bundle();
		exit(0);
	}


	/* first one is short to not let the user wait too long */
	init_display();
	show_tab(0);
	one_measurement(1);
	initialize_tuning();
	tuning_update_display();
	show_tab(0);



	while (!leave_powertop) {
		one_measurement(20);
		show_cur_tab();
		learn_parameters(15, 0);
	}

	endwin();
	printf("Leaving PowerTOP\n");


	end_process_data();
	end_cpu_data();

	save_all_results("saved_results.powertop");
	save_parameters("saved_parameters.powertop");
	learn_parameters(100, 0);
	save_parameters("saved_parameters.powertop");
	learn_parameters(100, 0);
	save_parameters("saved_parameters.powertop");
	learn_parameters(100, 0);
	save_parameters("saved_parameters.powertop");
	learn_parameters(100, 0);
	save_parameters("saved_parameters.powertop");
	learn_parameters(100, 0);
	save_parameters("saved_parameters.powertop");
	dump_parameter_bundle();
	return 0;

	
}
