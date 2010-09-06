#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include "cpu/cpu.h"
#include "process/process.h"
#include "perf/perf.h"
#include "perf/perf_bundle.h"
#include "lib.h"

#include "devices/device.h"
#include "devices/backlight.h"
#include "devices/usb.h"
#include "measurement/measurement.h"
#include "parameters/parameters.h"

int main(int argc, char **argv)
{
	int i;

	system("/sbin/modprobe cpufreq_stats > /dev/null 2>&1");


	load_results("saved_results.powertop");
	load_parameters("saved_parameters.powertop");

	enumerate_cpus();
	create_all_devices();
	detect_power_meters();

	register_parameter("base power", 8.4);
	register_parameter("cpu-wakeups", 1.0);
	register_parameter("cpu-consumption", 1.0);

        learn_parameters();


	for (i = 0; i < 1; i++) {
		start_power_measurement();
		devices_start_measurement();
		start_process_measurement();
		start_cpu_measurement();


		cout << "measuring\n";
		sleep(4);


		end_cpu_measurement();
		end_process_measurement();
		devices_end_measurement();
		end_power_measurement();

		cout << "doing math \n";

		process_cpu_data();
		process_process_data();
		store_results();
	}

        learn_parameters();
		report_devices();


//	cout << "\n\n\n";

//	display_cpu_pstates();

	end_process_data();
	end_cpu_data();

//	display_cpu_cstates("<table>\n", "</table>\n", "<tr><td>","</td><td>", "</td></tr>\n");
//	display_cpu_pstates("<table>\n", "</table>\n", "<tr><td>","</td><td>", "</td></tr>\n");

	i = 0;
	while (i++ < 10) {
		create_all_usb_devices();
		start_power_measurement();
		devices_start_measurement();
		start_process_measurement();
		start_cpu_measurement();


		cout << "measuring " << i << "\n";
		sleep(20);


		end_cpu_measurement();
		end_process_measurement();
		devices_end_measurement();
		end_power_measurement();

		cout << "doing math \n";

		process_cpu_data();
		process_process_data();
		

		global_joules_consumed();
		compute_bundle();

		report_devices();
		store_results();
		end_process_data();
		end_cpu_data();
		learn_parameters(100);
	}

	end_process_data();
	end_cpu_data();



	save_all_results("saved_results.powertop");


	learn_parameters(500);
	save_parameters("saved_parameters.powertop");
	printf("Final estimate:\n");
	dump_parameter_bundle();
	return 0;

	
}
