#include <iostream>
#include <stdlib.h>

#include "cpu/cpu.h"
#include "perf/perf.h"


int main(int argc, char **argv)
{
	int i;
	class perf_event * event;

	system("/sbin/modprobe cpufreq_stats > /dev/null 2>&1");

	enumerate_cpus();

	event = new perf_event("vfs:dirty_inode");


	for (i = 0; i < 1; i++) {
		event->start();
		start_cpu_measurement();


		cout << "measuring\n";
		sleep(3);


		end_cpu_measurement();
		event->stop();
	}

	event->process(NULL);
//	display_cpu_cstates();
//	cout << "\n\n\n";
//	display_cpu_pstates();

//	display_cpu_cstates("<table>\n", "</table>\n", "<tr><td>","</td><td>", "</td></tr>\n");
//	display_cpu_pstates("<table>\n", "</table>\n", "<tr><td>","</td><td>", "</td></tr>\n");

	delete event;

	return 0;
}