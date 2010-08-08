#include <iostream>
#include <stdlib.h>

#include "cpu/cpu.h"


int main(int argc, char **argv)
{
	int i;
	system("/sbin/modprobe cpufreq_stats > /dev/null 2>&1");

	enumerate_cpus();


	for (i = 0; i < 1; i++) {
		start_cpu_measurement();

		cout << "measuring\n";
		sleep(3);

		end_cpu_measurement();
	}

	display_cpu_cstates();


	display_cpu_pstates();


	return 0;
}