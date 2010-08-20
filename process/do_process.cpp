#include "process.h"
#include "interrupt.h"
#include "timer.h"
#include "../lib.h"

#include <vector>
#include <algorithm>
#include <stack>

#include <stdio.h>
#include <string.h>

#include "../perf/perf_bundle.h"
#include "../perf/perf_event.h"

static  class perf_bundle * perf_events;


vector <class process *> cpu_cache;
vector <class interrupt *> interrupt_cache;

vector <class power_consumer *> all_power;

vector< stack<class power_consumer *> > cpu_stack;

vector<int> cpu_level;
vector<class power_consumer *> cpu_blame;

#define LEVEL_HARDIRQ	1
#define LEVEL_SOFTIRQ	2
#define LEVEL_TIMER	3
#define LEVEL_WAKEUP	4
#define LEVEL_PROCESS	5

static void push_consumer(unsigned int cpu, class power_consumer *consumer)
{
	if (cpu_stack.size() <= cpu) {
		cpu_stack.resize(cpu + 1);
	}

	cpu_stack[cpu].push(consumer);
}

static void pop_consumer(unsigned int cpu)
{
	if (cpu_stack.size() > cpu) 
		cpu_stack[cpu].pop();
}

static class power_consumer *current_consumer(unsigned int cpu)
{
	if (cpu_stack.size() <= cpu)
		return NULL;
	return cpu_stack[cpu].top();
	
}


static void change_blame(unsigned int cpu, class power_consumer *consumer, int level)
{
	if (cpu_level.size() <= cpu) {
		cpu_level.resize(cpu + 1);
	}
	if (cpu_blame.size() <= cpu) {
		cpu_blame.resize(cpu + 1);
	}

	if (cpu_level[cpu] >= level)
		return;
	cpu_blame[cpu] = consumer;
	cpu_level[cpu] = level;
}

static void consume_blame(unsigned int cpu)
{
	if (cpu_level.size() <= cpu)
		return;
	if (cpu_blame.size() <= cpu)
		return;
	if (!cpu_blame[cpu])
		return;

	cpu_blame[cpu]->wake_ups++;
	cpu_blame[cpu] = NULL;
	cpu_level[cpu] = 0;
}

class perf_process_bundle: public perf_bundle
{
        virtual void handle_trace_point(int type, void *trace, int cpu, uint64_t time, unsigned char flags);
};





void perf_process_bundle::handle_trace_point(int type, void *trace, int cpu, uint64_t time, unsigned char flags)
{
	const char *event_name;

	if (type >= (int)event_names.size())
		return;
	event_name = event_names[type];

	if (strcmp(event_name, "sched:sched_switch") == 0) {
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
		new_proc->waker = NULL;


	}
	if (strcmp(event_name, "sched:sched_wakeup") == 0) {
		struct wakeup_entry *we;
		class power_consumer *from = NULL;
		class process *dest_proc;
		
		we = (struct wakeup_entry *)trace;

		if ( (flags & TRACE_FLAG_HARDIRQ) || (flags & TRACE_FLAG_SOFTIRQ)) {
			/* woken from interrupt */
			/* TODO: find the current irq handler and set "from" to that */
		} else {
			if ((int)cpu_cache.size() > cpu)
				from = cpu_cache[cpu];
		}

		dest_proc = find_create_process(we->comm, we->pid);
		
		if (!dest_proc->running && dest_proc->waker == NULL)
			dest_proc->waker = from;

	}
	if (strcmp(event_name, "irq:irq_handler_entry") == 0) {
		struct irq_entry *irqe;
		class interrupt *irq;
		irqe = (struct irq_entry *)trace;
		int Q;

		irq = find_create_interrupt(irqe->handler, irqe->irq, cpu);

		Q = (irqe->irq << 8) + cpu;
		if (Q >= (int)interrupt_cache.size())
			interrupt_cache.resize(Q + 1, NULL);
		interrupt_cache[Q] = irq;

		irq->start_interrupt(time);
		push_consumer(cpu, irq);		
		change_blame(cpu, irq, LEVEL_HARDIRQ);
	}

	if (strcmp(event_name, "irq:irq_handler_exit") == 0) {
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
		pop_consumer(cpu);
	}

	if (strcmp(event_name, "irq:softirq_entry") == 0) {
		int Q;
		struct softirq_entry *irqe;
		class interrupt *irq;
		irqe = (struct softirq_entry *)trace;
		const char *handler = NULL;

		Q = (255 << 8) + cpu;

		if (irqe->vec <= 9)
			handler = softirqs[irqe->vec];
		
		if (!handler)
			return;

		irq = find_create_interrupt(handler, irqe->vec, cpu);

		if (Q >= (int)interrupt_cache.size())
			interrupt_cache.resize(Q + 1, NULL);
		interrupt_cache[Q] = irq;

		irq->start_interrupt(time);
		push_consumer(cpu, irq);
	}
	if (strcmp(event_name, "irq:softirq_exit") == 0) {
		struct softirq_entry *irqe;
		irqe = (struct softirq_entry *)trace;
		class interrupt *irq;
		int Q;

		Q = (255 << 8) + cpu;

		if ((int)interrupt_cache.size() > Q) {
			irq = interrupt_cache[Q];
			if (irq)
				irq->end_interrupt(time);
		}
		pop_consumer(cpu);
	}
	if (strcmp(event_name, "timer:timer_start") == 0) {
		struct timer_start *tmr;
		tmr = (struct timer_start *)trace;
		timer_arm( (uint64_t)tmr->timer, (uint64_t)tmr->function);
	}
	if (strcmp(event_name, "timer:timer_cancel") == 0) {
		struct timer_cancel *tmr;
		tmr = (struct timer_cancel *)trace;
		timer_cancel( (uint64_t)tmr->timer);
	}
	if (strcmp(event_name, "timer:timer_expire_entry") == 0) {
		class timer *timer;
		struct timer_expire *tmr;
		tmr = (struct timer_expire *)trace;

		timer = timer_fire( (uint64_t)tmr->timer, (uint64_t)tmr->function, time);
		push_consumer(cpu, timer);
		change_blame(cpu, timer, LEVEL_TIMER);
	}
	if (strcmp(event_name, "timer:timer_expire_exit") == 0) {
		struct timer_cancel *tmr;
		tmr = (struct timer_cancel *)trace;
		timer_done( (uint64_t)tmr->timer, time);
		pop_consumer(cpu);
	}
}

void start_process_measurement(void)
{
	if (!perf_events) {
		perf_events = new perf_process_bundle();
		perf_events->add_event("sched:sched_switch");
		perf_events->add_event("sched:sched_wakeup");
		perf_events->add_event("irq:irq_handler_entry");
		perf_events->add_event("irq:irq_handler_exit");
		perf_events->add_event("irq:softirq_entry");
		perf_events->add_event("irq:softirq_exit");
		perf_events->add_event("timer:timer_start");
		perf_events->add_event("timer:timer_expire_entry");
		perf_events->add_event("timer:timer_expire_exit");
		perf_events->add_event("timer:timer_cancel");
	}

	perf_events->start();
}

void end_process_measurement(void)
{
	if (!perf_events)
		return;

	perf_events->stop();
}


static bool power_cpu_sort(class power_consumer * i, class power_consumer * j)
{
        return (i->Witts() > j->Witts());
}

void process_process_data(void)
{
	unsigned int i;
	if (!perf_events)
		return;


	printf("There are %i processes and %i interrupts \n",
		all_processes.size(),  all_interrupts.size());

	/* clean out old data */
	for (i = 0; i < all_processes.size() ; i++)
		delete all_processes[i];

	all_processes.erase(all_processes.begin(), all_processes.end());;
	cpu_cache.resize(0);

	for (i = 0; i < all_interrupts.size() ; i++)
		delete all_interrupts[i];

	all_interrupts.resize(0);
	interrupt_cache.resize(0);
	all_power.resize(0);




	/* process data */
	perf_events->process();
	perf_events->clear();

	merge_processes();

	all_processes_to_all_power();
	all_interrupts_to_all_power();
	all_timers_to_all_power();

	sort(all_power.begin(), all_power.end(), power_cpu_sort);
	for (i = 0; i < all_power.size() ; i++)
		printf("%s\n", all_power[i]->description());

}



void end_process_data(void)
{
	unsigned int i;
	/* clean out old data */
	for (i = 0; i < all_processes.size() ; i++)
		delete all_processes[i];

	all_processes.erase(all_processes.begin(), all_processes.end());;
	cpu_cache.resize(0);

	for (i = 0; i < all_interrupts.size() ; i++)
		delete all_interrupts[i];

	all_interrupts.resize(0);
	interrupt_cache.resize(0);
	all_power.resize(0);

	perf_events->clear();

}
