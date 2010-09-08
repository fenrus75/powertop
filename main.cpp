#include <iostream>
#include <stdlib.h>
#include <stdio.h>

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

void one_measurement(int seconds)
{
	create_all_usb_devices();
	start_power_measurement();
	devices_start_measurement();
	start_process_measurement();
	start_cpu_measurement();

	cout << "measuring for " << seconds << " seconds\n";
	sleep(20);

	end_cpu_measurement();
	end_process_measurement();
	devices_end_measurement();
	end_power_measurement();

	process_cpu_data();
	process_process_data();
	end_process_data();
		

	global_joules_consumed();
	compute_bundle();

	report_devices();
	store_results();
	end_cpu_data();
}

int main(int argc, char **argv)
{
	int i;

	system("/sbin/modprobe cpufreq_stats > /dev/null 2>&1");


	load_results("saved_results.powertop");
	load_parameters("saved_parameters.powertop");

	enumerate_cpus();
	create_all_devices();
	detect_power_meters();

	register_parameter("base power", 100);
	register_parameter("cpu-wakeups");
	register_parameter("cpu-consumption");
        learn_parameters();
	dump_parameter_bundle();
	save_parameters("saved_parameters.powertop");

	if (argc > 1) {
		if (strcmp(argv[1], "--calibrate") == 0)
			calibrate();
	}

	/* first one is short to not let the user wait too long */
	one_measurement(5);

	for (i = 0; i < 20; i++) {
		one_measurement(20);
		learn_parameters(10);
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
