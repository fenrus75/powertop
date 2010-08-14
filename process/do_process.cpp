#include "process.h"

#include <stdio.h>
#include <string.h>

#include "../perf/perf_bundle.h"

static  class perf_bundle * perf_events;


class perf_process_bundle: public perf_bundle
{
        virtual void handle_trace_point(int type, void *trace, int cpu, uint64_t time);
};



#define TASK_COMM_LEN 16
struct sched_switch {
	char prev_comm[TASK_COMM_LEN];
	int  prev_pid;
	int  prev_prio;
	long prev_state; /* Arjan weeps. */
	char next_comm[TASK_COMM_LEN];
	int  next_pid;
	int  next_prio;
};

struct irq_entry {
	int irq;
	int len;
	char handler[16];
};

struct irq_exit {
	int irq;
	int ret;
};

void perf_process_bundle::handle_trace_point(int type, void *trace, int cpu, uint64_t time)
{
	const char *event_name;

	if (type >= (int)event_names.size())
		return;
	event_name = event_names[type];

	if (strcmp(event_name, "sched:sched_switch")==0) {
		struct sched_switch *sw;
		sw = (struct sched_switch *)trace;
		printf("%03i  %08llu	<---- %s \n", cpu, time, sw->prev_comm);
		printf("%03i  %08llu	                  -----> %s \n", cpu, time, sw->next_comm);
	}
	if (strcmp(event_name, "irq:irq_handler_entry")==0) {
		struct irq_entry *irqe;
		irqe = (struct irq_entry *)trace;
		printf("%03i  %08llu  IRQ %i  %s \n", cpu, time, irqe->irq, irqe->handler);
	}

	if (strcmp(event_name, "irq:irq_handler_exit")==0) {
		struct irq_exit *irqe;
		irqe = (struct irq_exit *)trace;
		printf("%03i  %08llu  IRQ %i  returns %i \n", cpu, time, irqe->irq, irqe->ret);
	}
}

void start_process_measurement(void)
{
	if (!perf_events) {
		perf_events = new perf_process_bundle();
		perf_events->add_event("sched:sched_switch");
		perf_events->add_event("irq:irq_handler_entry");
		perf_events->add_event("irq:irq_handler_exit");
	}

	perf_events->start();
}

void end_process_measurement(void)
{
	if (!perf_events)
		return;

	perf_events->stop();
}


void process_process_data(void)
{
	if (!perf_events)
		return;

	perf_events->process();

}

