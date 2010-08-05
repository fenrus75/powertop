#include <iostream>

#include "cpu/cpu.h"


int main(int argc, char **argv)
{
	enumerate_cpus();


	start_cpu_measurement();

	cout << "measuring\n";
	sleep(3);

	end_cpu_measurement();


	display_cpus2();
	return 0;
}