#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "interrupt.h"


const char* softirqs[] = {
	"HI_SOFTIRQ",
	"timer(softirq)",
	"net tx(softirq)",
	"net_rx(softirq)",
	"block(softirq)",
	"block_iopoll(softirq)",
	"tasklet(softirq)",
	"sched(softirq)",
	"hrtimer(softirq)",
	"RCU(softirq)",
	NULL
};


interrupt::interrupt(const char *_handler, int _number)
{
	running_since = 0;
	number = _number;
	strncpy(handler, _handler, 31);
	wake_ups = 0;
	disk_hits = 0;
	accumulated_runtime = 0;
	child_runtime = 0;
	waker = NULL;
	raw_count = 0;
}


vector <class interrupt *> all_interrupts;

void interrupt::start_interrupt(uint64_t time)
{
	running_since = time;
	raw_count ++;
}

uint64_t interrupt::end_interrupt(uint64_t time)
{
	uint64_t delta;

	delta = time - running_since;
	accumulated_runtime += delta;
	return delta;
}

const char * interrupt::description(void)
{
	if (child_runtime > accumulated_runtime)
		child_runtime = 0;
	sprintf(desc, "Interrupt (%2i) %15s      time  %5.2fms    wakeups %3i  (child %5.1fms) (total: %i) ", number,
			handler,  (accumulated_runtime - child_runtime) / 1000000.0, wake_ups, 
				child_runtime / 1000000.0, raw_count);
	return desc;
}


class interrupt * find_create_interrupt(const char *_handler, int nr, int cpu)
{
	char handler[64];
	unsigned int i;
	class interrupt *new_irq;

	strcpy(handler, _handler);
	if (strcmp(handler, "timer")==0)
		sprintf(handler, "timer/%i", cpu);
		

	for (i = 0; i < all_interrupts.size(); i++) {
		if (all_interrupts[i] && all_interrupts[i]->number == nr && strcmp(handler, all_interrupts[i]->handler) == 0)
			return all_interrupts[i];
	}

	new_irq = new class interrupt(handler, nr);
	all_interrupts.push_back(new_irq);
	return new_irq;
}

void all_interrupts_to_all_power(void)
{
	unsigned int i;
	for (i = 0; i < all_interrupts.size() ; i++)
		if (all_interrupts[i]->accumulated_runtime)
			all_power.push_back(all_interrupts[i]);
}
