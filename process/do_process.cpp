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

vector <class power_consumer *> all_power;

vector< vector<class power_consumer *> > cpu_stack;

vector<int> cpu_level;
vector<int> cpu_credit;
vector<class power_consumer *> cpu_blame;

#define LEVEL_HARDIRQ	1
#define LEVEL_SOFTIRQ	2
#define LEVEL_TIMER	3
#define LEVEL_WAKEUP	4
#define LEVEL_PROCESS	5

static void push_consumer(unsigned int cpu, class power_consumer *consumer)
{
	if (cpu_stack.size() <= cpu)
		cpu_stack.resize(cpu + 1);
	cpu_stack[cpu].push_back(consumer);
}

static void pop_consumer(unsigned int cpu)
{
	if (cpu_stack.size() <= cpu)
		cpu_stack.resize(cpu + 1);

	if (cpu_stack[cpu].size())
		cpu_stack[cpu].resize(cpu_stack[cpu].size()-1);
}

static int consumer_depth(unsigned int cpu)
{
	if (cpu_stack.size() <= cpu)
		cpu_stack.resize(cpu + 1);
	return cpu_stack[cpu].size();
}

static class power_consumer *current_consumer(unsigned int cpu)
{
	if (cpu_stack.size() <= cpu)
		cpu_stack.resize(cpu + 1);
	if (cpu_stack[cpu].size())

		return cpu_stack[cpu][cpu_stack[cpu].size()-1];

	return NULL;
}

static void consumer_child_time(unsigned int cpu, uint64_t time)
{
	unsigned int i;
	if (cpu_stack.size() <= cpu)
		cpu_stack.resize(cpu + 1);
	for (i = 0; i < cpu_stack[cpu].size(); i++) 
		cpu_stack[cpu][i]->child_runtime += time;
}

static void set_wakeup_pending(unsigned int cpu)
{
	if (cpu_credit.size() <= cpu)
		cpu_credit.resize(cpu + 1);

	cpu_credit[cpu] = 1;
}

static void clear_wakeup_pending(unsigned int cpu)
{
	if (cpu_credit.size() <= cpu)
		cpu_credit.resize(cpu + 1);

	cpu_credit[cpu] = 0;
}

static int get_wakeup_pending(unsigned int cpu)
{
	if (cpu_credit.size() <= cpu)
		cpu_credit.resize(cpu + 1);
	return cpu_credit[cpu];
}

static void change_blame(unsigned int cpu, class power_consumer *consumer, int level)
{
	if (cpu_level[cpu] >= level)
		return;
	cpu_blame[cpu] = consumer;
	cpu_level[cpu] = level;
}

static void consume_blame(unsigned int cpu)
{
	if (!get_wakeup_pending(cpu))
		return;
	if (cpu_level.size() <= cpu)
		return;
	if (cpu_blame.size() <= cpu)
		return;
	if (!cpu_blame[cpu])
		return;

	cpu_blame[cpu]->wake_ups++;
	cpu_blame[cpu] = NULL;
	cpu_level[cpu] = 0;
	clear_wakeup_pending(cpu);
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


		/* find new process pointer */
		new_proc = find_create_process(sw->next_comm, sw->next_pid);

		/* find the old process pointer */

		while  (consumer_depth(cpu) > 1) {
			pop_consumer(cpu);
		}

		if (consumer_depth(cpu) == 1)
			old_proc = (class process *)current_consumer(cpu);		

		if (old_proc && strcmp(old_proc->name(), "process"))
			old_proc = NULL;

		/* retire the old process */

		if (old_proc)
			old_proc->deschedule_thread(time, sw->prev_pid);

		if (consumer_depth(cpu))
			pop_consumer(cpu);

		push_consumer(cpu, new_proc);

//		printf("Switch from %s to %s\n", sw->prev_comm, sw->next_comm);


		/* start new process */
		new_proc->schedule_thread(time, sw->next_pid);

		if (strncmp(sw->next_comm,"migration/", 10)) {
			if (sw->next_pid)
				change_blame(cpu, new_proc, LEVEL_PROCESS);

			consume_blame(cpu);
		}
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

		}

		dest_proc = find_create_process(we->comm, we->pid);
		
		if (!dest_proc->running && dest_proc->waker == NULL)
			dest_proc->waker = from;

	}
	if (strcmp(event_name, "irq:irq_handler_entry") == 0) {
		struct irq_entry *irqe;
		class interrupt *irq;
		irqe = (struct irq_entry *)trace;

		irq = find_create_interrupt(irqe->handler, irqe->irq, cpu);

		push_consumer(cpu, irq);

		irq->start_interrupt(time);

	}

	if (strcmp(event_name, "irq:irq_handler_exit") == 0) {
		struct irq_exit *irqe;
		class interrupt *irq;
		uint64_t t;

		irqe = (struct irq_exit *)trace;


		/* find interrupt (top of stack) */
		irq = (class interrupt *)current_consumer(cpu);
		if (irq && strcmp(irq->name(), "interrupt"))
			return;
		pop_consumer(cpu);
		/* retire interrupt */
		t = irq->end_interrupt(time);
		consumer_child_time(cpu, t);
	}

	if (strcmp(event_name, "irq:softirq_entry") == 0) {
		struct softirq_entry *irqe;
		class interrupt *irq;

		irqe = (struct softirq_entry *)trace;

		const char *handler = NULL;

		if (irqe->vec <= 9)
			handler = softirqs[irqe->vec];
		
		if (!handler)
			return;

		irq = find_create_interrupt(handler, irqe->vec, cpu);

		push_consumer(cpu, irq);

		irq->start_interrupt(time);
	}
	if (strcmp(event_name, "irq:softirq_exit") == 0) {
		struct softirq_entry *irqe;
		irqe = (struct softirq_entry *)trace;
		class interrupt *irq;
		uint64_t t;

		irq = (class interrupt *) current_consumer(cpu);
		if (irq && strcmp(irq->name(), "interrupt"))
			return;
		pop_consumer(cpu);
		/* pop irq */
		t = irq->end_interrupt(time);
		consumer_child_time(cpu, t);
	}
	if (strcmp(event_name, "timer:timer_expire_entry") == 0) {
		struct timer_expire *tmr;
		class timer *timer;
		tmr = (struct timer_expire *)trace;

		timer = find_create_timer((uint64_t)tmr->function);
		push_consumer(cpu, timer);
		timer->fire(time, (uint64_t)tmr->timer);


		if (strcmp(timer->handler, "delayed_work_timer_fn"))
			change_blame(cpu, timer, LEVEL_TIMER);
	}
	if (strcmp(event_name, "timer:timer_expire_exit") == 0) {
		class timer *timer;
		struct timer_cancel *tmr;
		uint64_t t;
		tmr = (struct timer_cancel *)trace;

		timer = (class timer *) current_consumer(cpu);
		if (timer && strcmp(timer->name(), "timer")) {
			printf("not a timer\n");
			return;
		}
		pop_consumer(cpu);
		t = timer->done(time, (uint64_t)tmr->timer);
		consumer_child_time(cpu, t);
	}
	if (strcmp(event_name, "timer:hrtimer_expire_entry") == 0) {
		struct hrtimer_expire *tmr;
		class timer *timer;
		tmr = (struct hrtimer_expire *)trace;

		timer = find_create_timer((uint64_t)tmr->function);

		push_consumer(cpu, timer);
		timer->fire(time, (uint64_t)tmr->timer);


		if (strcmp(timer->handler, "delayed_work_timer_fn"))
			change_blame(cpu, timer, LEVEL_TIMER);
	}
	if (strcmp(event_name, "timer:hrtimer_expire_exit") == 0) {
		class timer *timer;
		struct timer_cancel *tmr;
		uint64_t t;
		tmr = (struct timer_cancel *)trace;

		timer = (class timer *) current_consumer(cpu);
		if (timer && strcmp(timer->name(), "timer")) {
			printf("not a timer\n");
			return;
		}
		pop_consumer(cpu);
		t = timer->done(time, (uint64_t)tmr->timer);
		consumer_child_time(cpu, t);
	}
	if (strcmp(event_name, "power:power_start") == 0) {
		set_wakeup_pending(cpu);
	}
	if (strcmp(event_name, "power:power_end") == 0) {
		consume_blame(cpu);
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
		perf_events->add_event("timer:timer_expire_entry");
		perf_events->add_event("timer:timer_expire_exit");
		perf_events->add_event("timer:hrtimer_expire_entry");
		perf_events->add_event("timer:hrtimer_expire_exit");
		perf_events->add_event("power:power_start");
		perf_events->add_event("power:power_end");
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


	/* clean out old data */
	for (i = 0; i < all_processes.size() ; i++)
		delete all_processes[i];

	all_processes.erase(all_processes.begin(), all_processes.end());;

	for (i = 0; i < all_interrupts.size() ; i++)
		delete all_interrupts[i];

	all_interrupts.resize(0);
	all_power.resize(0);


	cpu_credit.resize(get_max_cpu()+1, 0);
	cpu_level.resize(get_max_cpu()+1, 0);
	cpu_blame.resize(get_max_cpu()+1, NULL);



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

	for (i = 0; i < all_interrupts.size() ; i++)
		delete all_interrupts[i];

	all_interrupts.resize(0);
	all_power.resize(0);

	perf_events->clear();

}
