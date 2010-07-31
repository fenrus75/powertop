#include <iostream>

#include "cpu.h"


int main(int argc, char **argv)
{
	enumerate_cpus();


	start_cpu_measurement();

	sleep(3);

	end_cpu_measurement();


	display_cpus();
	return 0;
}