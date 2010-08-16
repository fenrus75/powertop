#include "process.h"
#include "interrupt.h"

#include <vector>
#include <algorithm>

#include <stdio.h>
#include <string.h>

#include "../perf/perf_bundle.h"

static  class perf_bundle * perf_events;


vector <class process *> all_processes;
vector <class process *> cpu_cache;
vector <class interrupt *> all_interrupts;
vector <class interrupt *> interrupt_cache;

vector <class power_consumer *> all_power;

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

static class interrupt * find_create_interrupt(char *_handler, int nr, int cpu)
{
	char handler[64];
	unsigned int i;
	class interrupt *new_irq;

	strcpy(handler, _handler);
	if (strcmp(handler, "timer")==0)
		sprintf(handler, "timer/%i", cpu);
		

	for (i = 0; i < all_interrupts.size(); i++) {
		if (all_interrupts[i]->number == nr && strcmp(handler, all_interrupts[i]->handler) == 0)
			return all_interrupts[i];
	}

	new_irq = new class interrupt(handler, nr);
	all_interrupts.push_back(new_irq);
	return new_irq;
}

void perf_process_bundle::handle_trace_point(int type, void *trace, int cpu, uint64_t time)
{
	const char *event_name;

	if (type >= (int)event_names.size())
		return;
	event_name = event_names[type];

	if (strcmp(event_name, "sched:sched_switch")==0) {
		int from_idle = 0;
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

		from_idle = 0;
		if (!old_proc) {
			from_idle = 1;
			if (new_proc->is_idle)
				return;
		}

		new_proc->schedule_thread(time, sw->next_pid, from_idle);

		/* stick process in cpu cache, expand as needed */

		if ((int)cpu_cache.size() <= cpu)
			cpu_cache.resize(cpu + 1, NULL);
		cpu_cache[cpu] = new_proc;

	}
	if (strcmp(event_name, "irq:irq_handler_entry")==0) {
		struct irq_entry *irqe;
		class interrupt *irq;
		int from_idle = 0;
		irqe = (struct irq_entry *)trace;
		int Q;

		irq = find_create_interrupt(irqe->handler, irqe->irq, cpu);

		Q = (irqe->irq << 8) + cpu;
		if (Q >= (int)interrupt_cache.size())
			interrupt_cache.resize(Q + 1, NULL);
		interrupt_cache[Q] = irq;

		if ((int)cpu_cache.size() > cpu) {
			if (!cpu_cache[cpu])
				from_idle = 1;
			else if (cpu_cache[cpu]->is_idle)
				from_idle = 1;
		}

		irq->start_interrupt(time, from_idle);
	}

	if (strcmp(event_name, "irq:irq_handler_exit")==0) {
		struct irq_exit *irqe;
		class interrupt *irq;
		irqe = (struct irq_exit *)trace;
		int Q;

		Q = (irqe->irq << 8) + cpu;

		if ((int)interrupt_cache.size() > Q) {
			irq = interrupt_cache[Q];
			if (irq)
				irq->end_interrupt(time);
		}
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

static void merge_process(class process *one, class process *two)
{
	one->accumulated_runtime += two->accumulated_runtime;
	one->wake_ups += two->wake_ups;
	one->disk_hits += two->disk_hits;

	two->accumulated_runtime = 0;
	two->wake_ups = 0;
	two->disk_hits = 0;
}


static bool power_cpu_sort(class power_consumer * i, class power_consumer * j)
{
        return (i->Witts() > j->Witts());
}

void process_process_data(void)
{
	unsigned int i, j;
	class process *one, *two;
	if (!perf_events)
		return;

	/* clean out old data */

	/* process data */
	perf_events->process();

	/* find dupes and add up */
	for (i = 0; i < all_processes.size() ; i++) {
		one = all_processes[i];
		for (j = i + 1; j < all_processes.size(); j++) {
			two = all_processes[j];
			if (strcmp(one->comm, two->comm) == 0)
				merge_process(one, two);
		}
	}

	for (i = 0; i < all_processes.size() ; i++)
		if (all_processes[i]->accumulated_runtime)
			all_power.push_back(all_processes[i]);

	for (i = 0; i < all_interrupts.size() ; i++)
		if (all_interrupts[i]->accumulated_runtime)
			all_power.push_back(all_interrupts[i]);

	sort(all_power.begin(), all_power.end(), power_cpu_sort);
	for (i = 0; i < all_power.size() ; i++)
		printf("%s\n", all_power[i]->description());

}

