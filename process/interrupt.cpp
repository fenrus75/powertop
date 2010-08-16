#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "interrupt.h"



interrupt::interrupt(const char *_handler, int _number)
{
	running_since = 0;
	number = _number;
	strncpy(handler, _handler, 31);
	wake_ups = 0;
	disk_hits = 0;
	accumulated_runtime = 0;
}


void interrupt::start_interrupt(uint64_t time, int from_idle)
{
	running_since = time;
	if (from_idle)
		wake_ups++;
}

void interrupt::end_interrupt(uint64_t time)
{
	uint64_t delta;

	delta = time - running_since;
	accumulated_runtime += delta;
}

double interrupt::Witts(void)
{
	double cost;

	cost = 0.1 * wake_ups + (accumulated_runtime / 1000000.0);

	return cost;
}

const char * interrupt::description(void)
{
	sprintf(desc, "Interrupt %s (%i)     time  %5.1fms    wakeups %i",
			handler, number, accumulated_runtime / 1000000.0, wake_ups);
	return desc;
}
