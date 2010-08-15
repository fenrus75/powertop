#include "process.h"

#include <vector>

#include <stdio.h>
#include <string.h>

#include "../perf/perf_bundle.h"

static  class perf_bundle * perf_events;


vector <class process *> all_processes;
vector <class process *> cpu_cache;

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


static class process * find_create_process(char *comm, int pid)
{
	unsigned int i;
	class process *new_proc;

	for (i = 0; i < all_processes.size(); i++) {
		if (all_processes[i]->pid == pid && strcmp(comm, all_processes[i]->comm) == 0)
			return all_processes[i];
	}

	new_proc = new class process(comm, pid);
	all_processes.push_back(new_proc);
	return new_proc;
}

void perf_process_bundle::handle_trace_point(int type, void *trace, int cpu, uint64_t time)
{
	const char *event_name;

	if (type >= (int)event_names.size())
		return;
	event_name = event_names[type];

	if (strcmp(event_name, "sched:sched_switch")==0) {
		struct sched_switch *sw;
		class process *old_proc = NULL;
		class process *new_proc  = NULL;
		sw = (struct sched_switch *)trace;

		/* retire old process, from cpu cache */
		if ((int)cpu_cache.size() > cpu) 
			old_proc = cpu_cache[cpu];

		if (old_proc)
			old_proc->deschedule_thread(time);

		/* if new is idle -> early exit, just clear CPU cache */
		if (sw->next_pid == 0) {
			if ((int)cpu_cache.size() > cpu)
				cpu_cache[cpu] = NULL;
			return;
		}

		/* find new process pointer */
		new_proc = find_create_process(sw->next_comm, sw->next_pid);

		/* start new process */

		new_proc->schedule_thread(time, sw->next_pid);

		/* stick process in cpu cache, expand as needed */

		if ((int)cpu_cache.size() <= cpu)
			cpu_cache.resize(cpu + 1, NULL);
		cpu_cache[cpu] = new_proc;

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

