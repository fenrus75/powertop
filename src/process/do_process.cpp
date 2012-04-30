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
#include "processdevice.h"
#include "../lib.h"
#include "../report.h"
#include "../devlist.h"

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
	virtual void handle_trace_point(void *trace, int cpu, uint64_t time);
};

static bool comm_is_xorg(char *comm)
{
	return strcmp(comm, "Xorg") == 0 || strcmp(comm, "X") == 0;
}

/* some processes shouldn't be blamed for the wakeup if they wake a process up... for now this is a hardcoded list */
int dont_blame_me(char *comm)
{
	if (comm_is_xorg(comm))
		return 1;
	if (strcmp(comm, "dbus-daemon") == 0)
		return 1;

	return 0;
}

static void dbg_printf_pevent_info(struct event_format *event, struct record *rec)
{
	static struct trace_seq s;

	event->pevent->print_raw = 1;
	trace_seq_init(&s);
	pevent_event_info(&s, event, rec);
	trace_seq_putc(&s, '\n');
	trace_seq_terminate(&s);
	fprintf(stderr, "%.*s", s.len, s.buffer);
	trace_seq_destroy(&s);
}

static char * get_pevent_field_str(void *trace, struct event_format *event, struct format_field *field)
{
	unsigned long long offset, len;
	if (field->flags & FIELD_IS_DYNAMIC) {
		offset = field->offset;
		len = field->size;
		offset = pevent_read_number(event->pevent, (char *)trace + offset, len);
		offset &= 0xffff;
		return (char *)trace + offset;
	}
	/** no __data_loc field type*/
	return (char *)trace + field->offset;
}

void perf_process_bundle::handle_trace_point(void *trace, int cpu, uint64_t time)
{
	struct event_format *event;
	struct record rec; /* holder */
	struct format_field *field;
	unsigned long long val;
	int type;
	int ret;

	rec.data = trace;

	type = pevent_data_type(perf_event::pevent, &rec);
	event = pevent_find_event(perf_event::pevent, type);
	if (!event)
		return;

	if (time < first_stamp)
		first_stamp = time;

	if (time > last_stamp) {
		last_stamp = time;
		measurement_time = (0.0001 + last_stamp - first_stamp) / 1000000000 ;
	}

	if (strcmp(event->name, "sched_switch") == 0) {
		class process *old_proc = NULL;
		class process *new_proc  = NULL;
		const char *next_comm;
		int next_pid;
		int prev_pid;

		field = pevent_find_any_field(event, "next_comm");
		if (!field || !(field->flags & FIELD_IS_STRING))
			return; /* ?? */
	
		next_comm = get_pevent_field_str(trace, event, field);

		ret = pevent_get_field_val(NULL, event, "next_pid", &rec, &val, 0);
		if (ret < 0)
			return;
		next_pid = (int)val;

		ret = pevent_get_field_val(NULL, event, "prev_pid", &rec, &val, 0);
		if (ret < 0)
			return;
		prev_pid = (int)val;

		/* find new process pointer */
		new_proc = find_create_process(next_comm, next_pid);

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
			old_proc->deschedule_thread(time, prev_pid);
			old_proc->waker = NULL;
		}

		if (consumer_depth(cpu))
			pop_consumer(cpu);

		push_consumer(cpu, new_proc);

		/* start new process */
		new_proc->schedule_thread(time, next_pid);

		if (strncmp(next_comm,"migration/", 10) && strncmp(next_comm,"kworker/", 8) && strncmp(next_comm, "kondemand/",10)) {
			if (next_pid) {
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
	else if (strcmp(event->name, "sched_wakeup") == 0) {
		class power_consumer *from = NULL;
		class process *dest_proc = NULL;  
		class process *from_proc = NULL; 
		const char *comm;
		int flags;
		int pid;

		ret = pevent_get_common_field_val(NULL, event, "flags", &rec, &val, 0);
		if (ret < 0)
			return;
		flags = (int)val;

		if ( (flags & TRACE_FLAG_HARDIRQ) || (flags & TRACE_FLAG_SOFTIRQ)) {
			class timer *timer;
			timer = (class timer *) current_consumer(cpu);
			if (timer && strcmp(timer->name(), "timer")==0) {
				if (strcmp(timer->handler, "delayed_work_timer_fn") &&
				    strcmp(timer->handler, "hrtimer_wakeup") &&
				    strcmp(timer->handler, "it_real_fn"))
					from = timer;
			}
			/* woken from interrupt */
			/* TODO: find the current irq handler and set "from" to that */
		} else {
			from = current_consumer(cpu);
		}


		field = pevent_find_any_field(event, "comm");

		if (!field || !(field->flags & FIELD_IS_STRING))
 			return; 

		comm = get_pevent_field_str(trace, event, field);

		ret = pevent_get_field_val(NULL, event, "pid", &rec, &val, 0);
		if (ret < 0)
			return;
		pid = (int)val;

		dest_proc = find_create_process(comm, pid);

		if (from && strcmp(from->name(), "process")!=0){
			/* not a process doing the wakeup */
			from = NULL;
			from_proc = NULL;
		} else {
			from_proc = (class process *) from;
		}

		if (from_proc && (dest_proc->running == 0) && (dest_proc->waker == NULL) && (pid != 0) && !dont_blame_me(from_proc->comm))
			dest_proc->waker = from;
		if (from)
			dest_proc->last_waker = from;

		/* Account processes that wake up X specially */
		if (from && dest_proc && comm_is_xorg(dest_proc->comm))
			from->xwakes ++ ;

	}
	else if (strcmp(event->name, "irq_handler_entry") == 0) {
		class interrupt *irq = NULL;
		const char *handler;
		int nr;

		field = pevent_find_any_field(event, "name");
		if (!field || !(field->flags & FIELD_IS_STRING))
			return; /* ?? */

		handler = get_pevent_field_str(trace, event, field);

		ret = pevent_get_field_val(NULL, event, "irq", &rec, &val, 0);
		if (ret < 0)
			return;
		nr = (int)val;

		irq = find_create_interrupt(handler, nr, cpu);


		push_consumer(cpu, irq);

		irq->start_interrupt(time);

		if (strstr(irq->handler, "timer") ==NULL)
			change_blame(cpu, irq, LEVEL_HARDIRQ);

	}

	else if (strcmp(event->name, "irq_handler_exit") == 0) {
		class interrupt *irq = NULL;
		uint64_t t;

		/* find interrupt (top of stack) */
		irq = (class interrupt *)current_consumer(cpu);
		if (!irq || strcmp(irq->name(), "interrupt"))
			return;
		pop_consumer(cpu);
		/* retire interrupt */
		t = irq->end_interrupt(time);
		consumer_child_time(cpu, t);
	}

	else if (strcmp(event->name, "softirq_entry") == 0) {
		class interrupt *irq = NULL;
		const char *handler = NULL;
		int vec;

		ret = pevent_get_field_val(NULL, event, "vec", &rec, &val, 0);
                if (ret < 0) {
                        fprintf(stderr, "softirq_entry event returned no vector number?\n");
                        return;
                }
		vec = (int)val;

		if (vec <= 9)
			handler = softirqs[vec];

		if (!handler)
			return;

		irq = find_create_interrupt(handler, vec, cpu);

		push_consumer(cpu, irq);

		irq->start_interrupt(time);
		change_blame(cpu, irq, LEVEL_SOFTIRQ);
	}
	else if (strcmp(event->name, "softirq_exit") == 0) {
		class interrupt *irq = NULL;
		uint64_t t;

		irq = (class interrupt *) current_consumer(cpu);
		if (!irq  || strcmp(irq->name(), "interrupt"))
			return;
		pop_consumer(cpu);
		/* pop irq */
		t = irq->end_interrupt(time);
		consumer_child_time(cpu, t);
	}
	else if (strcmp(event->name, "timer_expire_entry") == 0) {
		class timer *timer = NULL;
		uint64_t function;
		uint64_t tmr;

		ret = pevent_get_field_val(NULL, event, "function", &rec, &val, 0);
		if (ret < 0) {
			fprintf(stderr, "timer_expire_entry event returned no fucntion value?\n");
			return;
		}
		function = (uint64_t)val;

		timer = find_create_timer(function);

		if (timer->is_deferred())
			return;

		ret = pevent_get_field_val(NULL, event, "timer", &rec, &val, 0);
		if (ret < 0) {
			fprintf(stderr, "softirq_entry event returned no timer ?\n");
			return;
		}
		tmr = (uint64_t)val;

		push_consumer(cpu, timer);
		timer->fire(time, tmr);

		if (strcmp(timer->handler, "delayed_work_timer_fn"))
			change_blame(cpu, timer, LEVEL_TIMER);
	}
	else if (strcmp(event->name, "timer_expire_exit") == 0) {
		class timer *timer = NULL;
		uint64_t tmr;
		uint64_t t;

		ret = pevent_get_field_val(NULL, event, "timer", &rec, &val, 0);
		if (ret < 0)
			return;
		tmr = (uint64_t)val;

		timer = (class timer *) current_consumer(cpu);
		if (!timer || strcmp(timer->name(), "timer")) {
			return;
		}
		pop_consumer(cpu);
		t = timer->done(time, tmr);
		consumer_child_time(cpu, t);
	}
	else if (strcmp(event->name, "hrtimer_expire_entry") == 0) {
		class timer *timer = NULL;
		uint64_t function;
		uint64_t tmr;

		ret = pevent_get_field_val(NULL, event, "function", &rec, &val, 0);
		if (ret < 0)
			return;
		function = (uint64_t)val;

		timer = find_create_timer(function);

		ret = pevent_get_field_val(NULL, event, "hrtimer", &rec, &val, 0);
		if (ret < 0)
			return;
		tmr = (uint64_t)val;

		push_consumer(cpu, timer);
		timer->fire(time, tmr);

		if (strcmp(timer->handler, "delayed_work_timer_fn"))
			change_blame(cpu, timer, LEVEL_TIMER);
	}
	else if (strcmp(event->name, "hrtimer_expire_exit") == 0) {
		class timer *timer = NULL;
		uint64_t tmr;
		uint64_t t;

		timer = (class timer *) current_consumer(cpu);
		if (!timer || strcmp(timer->name(), "timer")) {
			return;
		}

		ret = pevent_get_field_val(NULL, event, "hrtimer", &rec, &val, 0);
		if (ret < 0)
			return;
		tmr = (uint64_t)val;

		pop_consumer(cpu);
		t = timer->done(time, tmr);
		consumer_child_time(cpu, t);
	}
	else if (strcmp(event->name, "workqueue_execute_start") == 0) {
		class work *work = NULL;
		uint64_t function;
		uint64_t wk;

		ret = pevent_get_field_val(NULL, event, "function", &rec, &val, 0);
		if (ret < 0)
			return;
		function = (uint64_t)val;

		ret = pevent_get_field_val(NULL, event, "work", &rec, &val, 0);
		if (ret < 0)
			return;
		wk = (uint64_t)val;

		work = find_create_work(function);


		push_consumer(cpu, work);
		work->fire(time, wk);


		if (strcmp(work->handler, "do_dbs_timer") != 0 && strcmp(work->handler, "vmstat_update") != 0)
			change_blame(cpu, work, LEVEL_WORK);
	}
	else if (strcmp(event->name, "workqueue_execute_end") == 0) {
		class work *work = NULL;
		uint64_t t;
		uint64_t wk;

		ret = pevent_get_field_val(NULL, event, "work", &rec, &val, 0);
		if (ret < 0)
			return;
		wk = (uint64_t)val;

		work = (class work *) current_consumer(cpu);
		if (!work || strcmp(work->name(), "work")) {
			return;
		}
		pop_consumer(cpu);
		t = work->done(time, wk);
		consumer_child_time(cpu, t);
	}
	else if (strcmp(event->name, "cpu_idle") == 0) {
		ret = pevent_get_field_val(NULL, event, "state", &rec, &val, 0);
		if (val == 4294967295)
			consume_blame(cpu);
		else
			set_wakeup_pending(cpu);
	}
	else if (strcmp(event->name, "power_start") == 0) {
		set_wakeup_pending(cpu);
	}
	else if (strcmp(event->name, "power_end") == 0) {
		consume_blame(cpu);
	}
	else if (strcmp(event->name, "i915_gem_ring_dispatch") == 0
	 || strcmp(event->name, "i915_gem_request_submit") == 0) {
		/* any kernel contains only one of the these tracepoints,
		 * the latter one got replaced by the former one */
		class power_consumer *consumer = NULL;
		int flags;

		ret = pevent_get_common_field_val(NULL, event, "flags", &rec, &val, 0);
		if (ret < 0)
			return;
		flags = (int)val;

		consumer = current_consumer(cpu);
		/* currently we don't count graphic requests submitted from irq contect */
		if ( (flags & TRACE_FLAG_HARDIRQ) || (flags & TRACE_FLAG_SOFTIRQ)) {
			consumer = NULL;
		}


		/* if we are X, and someone just woke us, account the GPU op to the guy waking us */
		if (consumer && strcmp(consumer->name(), "process")==0) {
			class process *proc = NULL;
			proc = (class process *) consumer;
			if (comm_is_xorg(proc->comm) && proc->last_waker) {
				consumer = proc->last_waker;
			}
		}



		if (consumer) {
			consumer->gpu_ops++;
		}
	}
	else if (strcmp(event->name, "writeback_inode_dirty") == 0) {
		static uint64_t prev_time;
		class power_consumer *consumer = NULL;
		int dev;

		consumer = current_consumer(cpu);

		ret = pevent_get_field_val(NULL, event, "dev", &rec, &val, 0);
		if (ret < 0)
			
			return;
		dev = (int)val;

		if (consumer && strcmp(consumer->name(),
			"process")==0 && dev > 0) {

			consumer->disk_hits++;

			/* if the previous inode dirty was > 1 second ago, it becomes a hard hit */
			if ((time - prev_time) > 1000000000)
				consumer->hard_disk_hits++;

			prev_time = time;
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
		perf_events->add_event("hrtimer_expire_entry");
		perf_events->add_event("hrtimer_expire_exit");
		if (!perf_events->add_event("power:cpu_idle")){
			perf_events->add_event("power:power_start");
			perf_events->add_event("power:power_end");
		}
		perf_events->add_event("workqueue:workqueue_execute_start");
		perf_events->add_event("workqueue:workqueue_execute_end");
		perf_events->add_event("i915:i915_gem_ring_dispatch");
		perf_events->add_event("i915:i915_gem_request_submit");
		perf_events->add_event("writeback:writeback_inode_dirty");
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

	if (equals(iW, jW)) {
		double iR, jR;

		iR = i->accumulated_runtime - i->child_runtime;
		jR = j->accumulated_runtime - j->child_runtime;

		if (equals(iR, jR))
			return i->wake_ups > j->wake_ups;
		return (iR > jR);
	}

        return (iW > jW);
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

double total_disk_hits(void)
{
	double total = 0;
	unsigned int i;
	for (i = 0; i < all_power.size() ; i++)
		total += all_power[i]->disk_hits;


	total = total / measurement_time;


	return total;
}


double total_hard_disk_hits(void)
{
	double total = 0;
	unsigned int i;
	for (i = 0; i < all_power.size() ; i++)
		total += all_power[i]->hard_disk_hits;


	total = total / measurement_time;


	return total;
}

double total_xwakes(void)
{
	double total = 0;
	unsigned int i;
	for (i = 0; i < all_power.size() ; i++)
		total += all_power[i]->xwakes;


	total = total / measurement_time;


	return total;
}

void process_update_display(void)
{
#ifndef DISABLE_NCURSES
	unsigned int i;
	WINDOW *win;
	double pw;
	int tl;

	int show_power;
	int need_linebreak = 0;

	sort(all_power.begin(), all_power.end(), power_cpu_sort);

	show_power = global_power_valid();

	win = get_ncurses_win("Overview");
	if (!win)
		return;

	wclear(win);

	wmove(win, 1,0);

#if 0
	double sum;
	calculate_params();
	sum = 0.0;
	sum += get_parameter_value("base power");
	for (i = 0; i < all_power.size(); i++) {
		sum += all_power[i]->Witts();
	}

	wprintw(win, _("Estimated power: %5.1f    Measured power: %5.1f    Sum: %5.1f\n\n"),
				all_parameters.guessed_power, global_joules_consumed(), sum);
#endif

	pw = global_joules_consumed();
	tl = global_time_left() / 60;
	if (pw > 0.0001) {
		char buf[32];
		wprintw(win, _("The battery reports a discharge rate of %sW\n"),
				fmt_prefix(pw, buf));
		need_linebreak = 1;
	}
	if (tl > 0 && pw > 0.0001) {
		wprintw(win, _("The estimated remaining time is %i minutes\n"), tl);
		need_linebreak = 1;
	}

	if (need_linebreak)
		wprintw(win, "\n");


	wprintw(win, "Summary: %3.1f wakeups/second,  %3.1f GPU ops/second, %3.1f VFS ops/sec and %3.1f%% CPU use\n\n",
		total_wakeups(), total_gpu_ops(), total_disk_hits(), total_cpu_time()*100);


	if (show_power)
		wprintw(win, _("Power est.      Usage       Events/s    Category       Description\n"));
	else
		wprintw(win, _("                Usage       Events/s    Category       Description\n"));

	for (i = 0; i < all_power.size(); i++) {
		char power[16];
		char name[20];
		char usage[20];
		char events[20];
		char descr[128];
		format_watts(all_power[i]->Witts(), power, 10);

		if (!show_power)
			strcpy(power, "          ");
		sprintf(name, "%s", all_power[i]->type());
		while (mbstowcs(NULL,name,0) < 14) strcat(name, " ");


		if (all_power[i]->events() == 0 && all_power[i]->usage() == 0 && all_power[i]->Witts() == 0)
			break;

		usage[0] = 0;
		if (all_power[i]->usage_units()) {
			if (all_power[i]->usage() < 1000)
				sprintf(usage, "%5.1f%s", all_power[i]->usage(), all_power[i]->usage_units());
			else
				sprintf(usage, "%5i%s", (int)all_power[i]->usage(), all_power[i]->usage_units());
		}
		while (mbstowcs(NULL,usage,0) < 14) strcat(usage, " ");
		sprintf(events, "%5.1f", all_power[i]->events());
		if (!all_power[i]->show_events())
			events[0] = 0;
		else if (all_power[i]->events() <= 0.3)
			sprintf(events, "%5.2f", all_power[i]->events());

		while (strlen(events) < 12) strcat(events, " ");
		wprintw(win, "%s  %s %s %s %s\n", power, usage, events, name, pretty_print(all_power[i]->description(), descr, 128));
	}
#endif // DISABLE_NCURSES
}

static const char *process_class(int line)
{
	if (line & 1) {
		return "process_odd";
	}
	return "process_even";
}

void report_process_update_display(void)
{
	unsigned int i, lines = 0;
	unsigned int total;

	int show_power;

	if ((!reportout.csv_report)&&(!reportout.http_report))
		return;

	sort(all_power.begin(), all_power.end(), power_cpu_sort);

	show_power = global_power_valid();

	if (reporttype){
		fprintf(reportout.http_report,
			"<div id=\"software\"><h2>Overview of Software Power Consumers</h2>\n <table width=\"100%%\">\n");
		if (show_power)
			fprintf(reportout.http_report,
				"<tr><th width=\"10%%\">Power est.</th><th width=\"10%%\">Usage</th><th width=\"10%%\">Wakeups/s</th><th width=\"10%%\">GPU ops/s</th><th width=\"10%%\">Disk IO/s</th><th width=\"10%%\">GFX Wakeups/s</th><th width=\"10%%\" class=\"process\">Category</th><th class=\"process\">Description</th></tr>\n");
		else
			fprintf(reportout.http_report,
				"<tr><th width=\"10%%\">Usage</th><th width=\"10%%\">Wakeups/s</th><th width=\"10%%\">GPU ops/s</th><th width=\"10%%\">Disk IO/s</th><th width=\"10%%\">GFX Wakeups/s</th><th width=\"10%%\" class=\"process\">Category</th><th class=\"process\">Description</th></tr>\n");
	}else {
		fprintf(reportout.csv_report,"**Overview of Software Power Consumers**, \n\n");
		if (show_power)
			fprintf(reportout.csv_report,
				"Power est., Usage, Wakeups, GPU ops, Disk IO, GFX Wakeups, Category, Description, \n");
		else
			fprintf(reportout.csv_report,
				"Usage,  Wakeups, GPU ops, Disk IO, GFX Wakeups, Category, Description, \n");
	}

	total = all_power.size();

	if (total > 100)
		total = 100;

	for (i = 0; i < total; i++) {
		char power[16];
		char name[20];
		char usage[20];
		char wakes[20];
		char gpus[20];
		char disks[20];
		char xwakes[20];
		char descr[128];
		format_watts(all_power[i]->Witts(), power, 10);


		if (!show_power)
			strcpy(power, "          ");
		sprintf(name, "%s", all_power[i]->type());

		if (strcmp(name, "Device") == 0)
			continue;

		lines++;

		if (all_power[i]->events() == 0 && all_power[i]->usage() == 0 && all_power[i]->Witts() == 0)
			break;

		usage[0] = 0;
		if (all_power[i]->usage_units()) {
			if (all_power[i]->usage() < 1000)
				sprintf(usage, "%5.1f%s", all_power[i]->usage(), all_power[i]->usage_units());
			else
				sprintf(usage, "%5i%s", (int)all_power[i]->usage(), all_power[i]->usage_units());
		}
		sprintf(wakes, "%5.1f", all_power[i]->wake_ups / measurement_time);
		if (all_power[i]->wake_ups / measurement_time <= 0.3)
			sprintf(wakes, "%5.2f", all_power[i]->wake_ups / measurement_time);
		sprintf(gpus, "%5.1f", all_power[i]->gpu_ops / measurement_time);
		sprintf(disks, "%5.1f (%5.1f)", all_power[i]->hard_disk_hits / measurement_time, all_power[i]->disk_hits / measurement_time);
		sprintf(xwakes, "%5.1f", all_power[i]->xwakes / measurement_time);
		if (!all_power[i]->show_events()) {
			wakes[0] = 0;
			gpus[0] = 0;
			disks[0] = 0;
		}

		if (all_power[i]->gpu_ops == 0)
			gpus[0] = 0;
		if (all_power[i]->wake_ups == 0)
			wakes[0] = 0;
		if (all_power[i]->disk_hits == 0)
			disks[0] = 0;
		if (all_power[i]->xwakes == 0)
			xwakes[0] = 0;

		if (show_power) {
			if (reporttype)
				fprintf(reportout.http_report,"<tr class=\"%s\"><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td></td><td class=\"process_power\">%s</td><td>%s</td><td>%s</td></tr>\n",
					process_class(lines), power, usage, wakes, gpus, disks, xwakes, name, pretty_print(all_power[i]->description(), descr, 128));
			else
				fprintf(reportout.csv_report,"%s, %s, %s, %s, %s, %s, %s, %s,\n",
					power, usage, wakes, gpus, disks, xwakes, name,
					pretty_print(all_power[i]->description(), descr, 128));

		} else {
			if (reporttype)
				fprintf(reportout.http_report,"<tr class=\"%s\"><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td><td>%s</td><td>%s</td></tr>\n",
					process_class(lines), usage, wakes, gpus, disks, xwakes, name, pretty_print(all_power[i]->description(), descr, 128));
			else
				fprintf(reportout.csv_report,"%s, %s, %s, %s, %s, %s, %s, \n",
					usage, wakes, gpus, disks, xwakes, name,
					pretty_print(all_power[i]->description(), descr, 128));
		}
	}
	if (reporttype)
		fprintf(reportout.http_report,"</table></div>\n");
	else
		fprintf(reportout.csv_report,"\n");
}

void report_summary(void)
{
	unsigned int i, lines = 0;
	unsigned int total;

	int show_power;

	if ((!reportout.csv_report)&&(!reportout.http_report))
		return;

	sort(all_power.begin(), all_power.end(), power_cpu_sort);

	show_power = global_power_valid();

	if (reporttype) {
		fprintf(reportout.http_report,
			"<div id=\"summary\"><h2>Power Consumption Summary</h2>\n");
		fprintf(reportout.http_report,
			"<p>%3.1f wakeups/second,  %3.1f GPU ops/second, %3.1f VFS ops/sec, %3.1f GFX wakes/sec and %3.1f%% CPU use</p>\n <table width=\"100%%\">\n",
			total_wakeups(), total_gpu_ops(), total_disk_hits(), total_xwakes(), total_cpu_time()*100);

		if (show_power)
			fprintf(reportout.http_report,
		"<tr><th width=\"10%%\">Power est.</th><th width=\"10%%\">Usage</th><th width=\"10%%\">Events/s</th><th width=\"10%%\" class=\"process\">Category</th><th class=\"process\">Description</th></tr>\n");
		else
		fprintf(reportout.http_report,
		"<tr><th width=\"10%%\">Usage</th><th width=\"10%%\">Events/s</th><th width=\"10%%\" class=\"process\">Category</th><th class=\"process\">Description</th></tr>\n");

	}else {
		fprintf(reportout.csv_report,
			"**Power Consumption Summary** \n");
		fprintf(reportout.csv_report,
			"%3.1f wakeups/second,  %3.1f GPU ops/second, %3.1f VFS ops/sec, %3.1f GFX wakes/sec and %3.1f%% CPU use \n\n",
			total_wakeups(), total_gpu_ops(), total_disk_hits(), total_xwakes(), total_cpu_time()*100);

		if (show_power)
			fprintf(reportout.csv_report,"Power est., Usage, Events/s, Category,  Description, \n");
        else
			fprintf(reportout.csv_report,"Usage, Events/s, Category, Description, \n");

	}
	total = all_power.size();
	if (total > 10)
		total = 10;

	for (i = 0; i < all_power.size(); i++) {
		char power[16];
		char name[20];
		char usage[20];
		char events[20];
		char descr[128];
		format_watts(all_power[i]->Witts(), power, 10);


		if (!show_power)
			strcpy(power, "          ");
		sprintf(name, "%s", all_power[i]->type());

		lines++;

		if (lines > total)
			break;

		if (all_power[i]->events() == 0 && all_power[i]->usage() == 0 && all_power[i]->Witts() == 0)
			break;

		usage[0] = 0;
		if (all_power[i]->usage_units()) {
			if (all_power[i]->usage() < 1000)
				sprintf(usage, "%5.1f%s", all_power[i]->usage_summary(), all_power[i]->usage_units_summary());
			else
				sprintf(usage, "%5i%s", (int)all_power[i]->usage_summary(), all_power[i]->usage_units_summary());
		}
		sprintf(events, "%5.1f", all_power[i]->events());
		if (!all_power[i]->show_events())
			events[0] = 0;
		else if (all_power[i]->events() <= 0.3)
			sprintf(events, "%5.2f", all_power[i]->events());

		if (show_power) {
			if (reporttype)
				fprintf(reportout.http_report,"<tr class=\"%s\"><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td><td>%s</td><td>%s</td></tr>\n",
					process_class(lines), power, usage, events, name, pretty_print(all_power[i]->description(), descr, 128));
			else
				fprintf(reportout.csv_report,"%s,%s,%s,%s,%s \n",
					power, usage, events, name, pretty_print(all_power[i]->description(), descr, 128));
		} else {
			if (reporttype)
				fprintf(reportout.http_report,
					"<tr class=\"%s\"><td class=\"process_power\">%s</td><td class=\"process_power\">%s</td><td>%s</td><td>%s</td></tr>\n",
					process_class(lines),usage, events, name, pretty_print(all_power[i]->description(), descr, 128));
			else
				fprintf(reportout.csv_report,"%s,%s,%s,%s, \n",
					usage, events, name, pretty_print(all_power[i]->description(), descr, 128));
		}
	}
	if (reporttype)
		fprintf(reportout.http_report,"</table></div>\n");
	else
		fprintf(reportout.csv_report,"\n");
}


void process_process_data(void)
{
	if (!perf_events)
		return;

	clear_processes();
	clear_interrupts();

	all_power.erase(all_power.begin(), all_power.end());
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

	run_devpower_list();

	merge_processes();

	all_processes_to_all_power();
	all_interrupts_to_all_power();
	all_timers_to_all_power();
	all_work_to_all_power();
	all_devices_to_all_power();

	sort(all_power.begin(), all_power.end(), power_cpu_sort);
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
	report_utilization("cpu-consumption", total_cpu_time());
	report_utilization("cpu-wakeups", total_wakeups());
	report_utilization("gpu-operations", total_gpu_ops());
	report_utilization("disk-operations", total_disk_hits());
	report_utilization("disk-operations-hard", total_hard_disk_hits());
	report_utilization("xwakes", total_xwakes());

	all_power.erase(all_power.begin(), all_power.end());
	clear_processes();
	clear_proc_devices();
	clear_interrupts();
	clear_timers();
	clear_work();
	clear_consumers();

	perf_events->clear();

}

void clear_process_data(void)
{
	if (perf_events)
		perf_events->release();
	delete perf_events;
}
