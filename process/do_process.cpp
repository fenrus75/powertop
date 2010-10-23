/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */
#include "process.h"
#include "interrupt.h"
#include "timer.h"
#include "work.h"
#include "device.h"
#include "../lib.h"

#include <vector>
#include <algorithm>
#include <stack>

#include <stdio.h>
#include <string.h>
#include <ncurses.h>

#include "../perf/perf_bundle.h"
#include "../perf/perf_event.h"
#include "../parameters/parameters.h"
#include "../display.h"
#include "../measurement/measurement.h"

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
#define LEVEL_WORK	6

static uint64_t first_stamp, last_stamp;

double measurement_time;

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

static void clear_consumers(void)
{
	unsigned int i;
	for (i = 0; i < cpu_stack.size(); i++)
		cpu_stack[i].resize(0);
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


/* some processes shouldn't be blamed for the wakeup if they wake a process up... for now this is a hardcoded list */
int dont_blame_me(char *comm)
{
	if (strcmp(comm, "Xorg"))
		return 1;
	if (strcmp(comm, "dbus-daemon"))
		return 1;

	return 0;
}



void perf_process_bundle::handle_trace_point(int type, void *trace, int cpu, uint64_t time, unsigned char flags)
{
	const char *event_name;

	if (type >= (int)event_names.size())
		return;
	event_name = event_names[type];

	if (time < first_stamp)
		first_stamp = time;

	if (time > last_stamp) {
		last_stamp = time;
		measurement_time = (0.0001 + last_stamp - first_stamp) / 1000000000 ;
	}

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

		if (old_proc) {
			old_proc->deschedule_thread(time, sw->prev_pid);
			old_proc->waker = NULL;
		}

		if (consumer_depth(cpu))
			pop_consumer(cpu);

		push_consumer(cpu, new_proc);

		/* start new process */
		new_proc->schedule_thread(time, sw->next_pid);

		if (strncmp(sw->next_comm,"migration/", 10) && strncmp(sw->next_comm,"kworker/", 8) && strncmp(sw->next_comm, "kondemand/",10)) {
			if (sw->next_pid) {
				/* If someone woke us up.. blame him instead */
				if (new_proc->waker) {
					change_blame(cpu, new_proc->waker, LEVEL_PROCESS);
				} else {
					change_blame(cpu, new_proc, LEVEL_PROCESS);
				}
			}

			consume_blame(cpu);
		}
		new_proc->waker = NULL;
	}
	if (strcmp(event_name, "sched:sched_wakeup") == 0) {
		struct wakeup_entry *we;
		class power_consumer *from = NULL;
		class process *dest_proc;
		
		we = (struct wakeup_entry *)trace;

		if ( (flags & TRACE_FLAG_HARDIRQ) || (flags & TRACE_FLAG_SOFTIRQ)) {
			class timer *timer;
			timer = (class timer *) current_consumer(cpu);
			if (timer && strcmp(timer->name(), "timer")==0) {
				if (strcmp(timer->handler, "delayed_work_timer_fn") && 
				    strcmp(timer->handler, "hrtimer_wakeup"))
					from = timer;
			}
			/* woken from interrupt */
			/* TODO: find the current irq handler and set "from" to that */
		} else {
			from = current_consumer(cpu);
		}

		dest_proc = find_create_process(we->comm, we->pid);

		if (from && strcmp(from->name(), "process")!=0){
			/* not a process doing the wakeup */
			from = NULL;
		}
		
		if (!dest_proc->running && dest_proc->waker == NULL && we->pid != 0 && !dont_blame_me(we->comm))
			dest_proc->waker = from;
		if (from)
			dest_proc->last_waker = from;

	}
	if (strcmp(event_name, "irq:irq_handler_entry") == 0) {
		struct irq_entry *irqe;
		class interrupt *irq;
		irqe = (struct irq_entry *)trace;

		irq = find_create_interrupt(irqe->handler, irqe->irq, cpu);

		push_consumer(cpu, irq);

		irq->start_interrupt(time);

		if (strstr(irq->handler, "timer") ==NULL)
			change_blame(cpu, irq, LEVEL_HARDIRQ);

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
		change_blame(cpu, irq, LEVEL_SOFTIRQ);
	}
	if (strcmp(event_name, "irq:softirq_exit") == 0) {
		struct softirq_entry *irqe;
		irqe = (struct softirq_entry *)trace;
		class interrupt *irq;
		uint64_t t;

		irq = (class interrupt *) current_consumer(cpu);
		if (!irq  || strcmp(irq->name(), "interrupt"))
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
		if (!timer || strcmp(timer->name(), "timer")) {
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
		if (!timer || strcmp(timer->name(), "timer")) {
			return;
		}
		pop_consumer(cpu);
		t = timer->done(time, (uint64_t)tmr->timer);
		consumer_child_time(cpu, t);
	}
	if (strcmp(event_name, "workqueue:workqueue_execute_start") == 0) {
		struct workqueue_start *wq;
		class work *work;
		wq = (struct workqueue_start *)trace;

		work = find_create_work((uint64_t)wq->function);

		push_consumer(cpu, work);
		work->fire(time, (uint64_t)wq->work);


		change_blame(cpu, work, LEVEL_WORK);
	}
	if (strcmp(event_name, "workqueue:workqueue_execute_end") == 0) {
		class work *work;
		struct workqueue_end *wq;
		uint64_t t;
		wq = (struct workqueue_end *)trace;

		work = (class work *) current_consumer(cpu);
		if (!work || strcmp(work->name(), "work")) {
			return;
		}
		pop_consumer(cpu);
		t = work->done(time, (uint64_t)wq->work);
		consumer_child_time(cpu, t);
	}
	if (strcmp(event_name, "power:power_start") == 0) {
		set_wakeup_pending(cpu);
	}
	if (strcmp(event_name, "power:power_end") == 0) {
		consume_blame(cpu);
	}
	if (strcmp(event_name, "i915:i915_gem_request_submit") == 0) {
		class power_consumer *consumer;
		consumer = current_consumer(cpu);
		/* currently we don't count graphic requests submitted from irq contect */
		if ( (flags & TRACE_FLAG_HARDIRQ) || (flags & TRACE_FLAG_SOFTIRQ)) {
			consumer = NULL;
		}


		/* if we are X, and someone just woke us, account the GPU op to the guy waking us */
		if (consumer && strcmp(consumer->name(), "process")==0) {
			class process *proc;
			proc = (class process *) consumer;
			if (strcmp(proc->comm, "Xorg")==0 && proc->last_waker) {
				consumer = proc->last_waker;
			}
		}
			
			

		if (consumer) {
			consumer->gpu_ops++;
		}
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
		perf_events->add_event("workqueue:workqueue_execute_start");
		perf_events->add_event("workqueue:workqueue_execute_end");
		perf_events->add_event("i915:i915_gem_request_submit");
	}

	first_stamp = ~0ULL;
	last_stamp = 0;
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
	double iW, jW;

	iW = i->Witts();
	jW = j->Witts();

	if (iW == jW) {
		double iR, jR;

		iR = i->accumulated_runtime - i->child_runtime;
		jR = j->accumulated_runtime - j->child_runtime;
 
		if (iR == jR)
			return i->wake_ups > j->wake_ups;
		return (iR > jR);
	}
	
        return (iW > jW);
}

void process_update_display(void)
{
	unsigned int i;
	WINDOW *win;
	double sum;

	int show_power;

	sort(all_power.begin(), all_power.end(), power_cpu_sort);

	show_power = global_power_valid();

	win = tab_windows["Overview"];
	if (!win)
		return;

	wclear(win);

	wmove(win, 2,0);

	calculate_params();
	sum = 0.0;
	sum += get_parameter_value("base power");	
	for (i = 0; i < all_power.size(); i++) {
		sum += all_power[i]->Witts();
	}

//	wprintw(win, "Estimated power: %5.1f    Measured power: %5.1f    Sum: %5.1f\n\n",
//				all_parameters.guessed_power, global_joules_consumed(), sum);


	if (show_power)
		wprintw(win, "Power est.    Usage/s   Events/s    Category       Description\n");
	else
		wprintw(win, "              Usage/s   Events/s    Category       Description\n");

	for (i = 0; i < all_power.size(); i++) {
		char power[16];
		char name[20];
		char usage[20];
		char events[20];
		format_watts(all_power[i]->Witts(), power, 10);

		if (!show_power)
			strcpy(power, "          ");
		sprintf(name, all_power[i]->type());
		while (strlen(name) < 14) strcat(name, " ");


		if (all_power[i]->events() == 0 && all_power[i]->usage() == 0 && all_power[i]->Witts() == 0)
			break;

		usage[0] = 0;
		if (all_power[i]->usage_units()) {
			if (all_power[i]->usage() < 1000) 
				sprintf(usage, "%5.1f%s", all_power[i]->usage(), all_power[i]->usage_units());
			else
				sprintf(usage, "%5i%s", (int)all_power[i]->usage(), all_power[i]->usage_units());
		}
		while (strlen(usage) < 10) strcat(usage, " ");
		sprintf(events, "%5.1f", all_power[i]->events());
		if (!all_power[i]->show_events())
			events[0] = 0;
		while (strlen(events) < 12) strcat(events, " ");
		wprintw(win, "%s  %s %s %s %s\n", power, usage, events, name, all_power[i]->description());
	}
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
	clear_consumers();


	cpu_credit.resize(0, 0);
	cpu_credit.resize(get_max_cpu()+1, 0);
	cpu_level.resize(0, 0);
	cpu_level.resize(get_max_cpu()+1, 0);
	cpu_blame.resize(0, NULL);
	cpu_blame.resize(get_max_cpu()+1, NULL);



	/* process data */
	perf_events->process();
	perf_events->clear();

	merge_processes();

	all_processes_to_all_power();
	all_interrupts_to_all_power();
	all_timers_to_all_power();
	all_work_to_all_power();
	all_devices_to_all_power();

	sort(all_power.begin(), all_power.end(), power_cpu_sort);

}

double total_wakeups(void)
{
	double total = 0;
	unsigned int i;
	for (i = 0; i < all_power.size() ; i++)
		total += all_power[i]->wake_ups;

	total = total / measurement_time;


	return total;
}

double total_gpu_ops(void)
{
	double total = 0;
	unsigned int i;
	for (i = 0; i < all_power.size() ; i++)
		total += all_power[i]->gpu_ops;


	total = total / measurement_time;


	return total;
}

double total_cpu_time(void)
{
	unsigned int i;
	double total = 0.0;
	for (i = 0; i < all_power.size() ; i++) {
		if (all_power[i]->child_runtime > all_power[i]->accumulated_runtime)
			all_power[i]->child_runtime = 0;
		total += all_power[i]->accumulated_runtime - all_power[i]->child_runtime;
	}

	
	total =  (total / (0.0001 + last_stamp - first_stamp));

	return total;
}



void end_process_data(void)
{
	unsigned int i;

	report_utilization("cpu-consumption", total_cpu_time());
	report_utilization("cpu-wakeups", total_wakeups());
	report_utilization("gpu-operations", total_gpu_ops());

	/* clean out old data */
	for (i = 0; i < all_processes.size() ; i++)
		all_processes[i]->wake_ups = 0;
	for (i = 0; i < all_processes.size() ; i++)
		all_processes[i]->accumulated_runtime = 0;
	for (i = 0; i < all_processes.size() ; i++)
		all_processes[i]->child_runtime = 0;

	for (i = 0; i < all_processes.size() ; i++)
		delete all_processes[i];

	for (i = 0; i < all_proc_devices.size() ; i++)
		delete all_proc_devices[i];

	all_processes.erase(all_processes.begin(), all_processes.end());;

	for (i = 0; i < all_interrupts.size() ; i++)
		delete all_interrupts[i];

	all_interrupts.resize(0);
	all_power.resize(0);
	all_proc_devices.resize(0);
	clear_timers();

	clear_consumers();

	perf_events->clear();

}
