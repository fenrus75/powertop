
#include "cpu.h"
#include "lib.h"

void cpu_linux::measurement_start(void)
{
	read_cstate_data(number, cstate_usage, cstate_duration, NULL);
}


void cpu_linux::measurement_end(void)
{
	read_cstate_data(number, cstate_usage_after, cstate_duration_after, NULL);
}


void cpu_linux::consolidate_children(void)
{
	// lowest level, nothing to consolidate
}

void cpu_linux::display(void)
{
	int i;
	cout << "\t\tCPU number " << number << "\n";

	for (i = 0; i < 8; i++) {
		cout << "\t\t\t C" << i << "  for " << (cstate_duration_after[i] - cstate_duration[i]) / 1000.0 << "s \n";
	}
}