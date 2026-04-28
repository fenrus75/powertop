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
#include "../report/report.h"
#include "../report/report-data-html.h"
#include "../report/report-maker.h"
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

std::vector <class power_consumer *> all_power;

std::vector< std::vector<class power_consumer *> > cpu_stack;

std::vector<int> cpu_level;
std::vector<int> cpu_credit;
std::vector<class power_consumer *> cpu_blame;

#define LEVEL_HARDIRQ	1
#define LEVEL_SOFTIRQ	2
#define LEVEL_TIMER	3
#define LEVEL_WAKEUP	4
#define LEVEL_PROCESS	5
#define LEVEL_WORK	6

static uint64_t first_stamp, last_stamp;
static uint64_t prev_disk_time;

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

	return nullptr;
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
	if (cpu_level.size() <= cpu)
		return;
	if (cpu_blame.size() <= cpu)
		return;
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
	cpu_blame[cpu] = nullptr;
	cpu_level[cpu] = 0;
	clear_wakeup_pending(cpu);
}


class perf_process_bundle: public perf_bundle
{
	virtual void handle_trace_point(void *trace, int cpu, uint64_t time) override;
};

static bool comm_is_xorg(const std::string &comm)
{
	return comm == "Xorg" || comm == "X";
}

/* some processes shouldn't be blamed for the wakeup if they wake a process up... for now this is a hardcoded list */
int dont_blame_me(const std::string &comm)
{
	if (comm_is_xorg(comm))
		return 1;
	if (comm == "dbus-daemon")
		return 1;

	return 0;
}

static std::string get_tep_field_str(void *trace, struct tep_event *event, struct tep_format_field *field)
{
	unsigned long long offset, len;
	if (field->flags & TEP_FIELD_IS_DYNAMIC) {
		offset = field->offset;
		len = field->size;
		offset = tep_read_number(event->tep, (char *)trace + offset, len);
		offset &= 0xffff;
		return (char *)trace + offset;
	}
	/** no __data_loc field type*/
	return (char *)trace + field->offset;
}

void perf_process_bundle::handle_trace_point(void *trace, int cpu, uint64_t time)
{
	struct tep_event *event;
	struct tep_record rec; /* holder */
	struct tep_format_field *field;
	unsigned long long val;
	int type;
	int ret;

	rec.data = trace;

	type = tep_data_type(perf_event::tep, &rec);
	event = tep_find_event(perf_event::tep, type);
	if (!event)
		return;

	std::string_view event_name(event->name);

	if (time < first_stamp)
		first_stamp = time;

	if (time > last_stamp) {
		last_stamp = time;
		measurement_time = (0.0001 + last_stamp - first_stamp) / 1000000000 ;
	}

	if (event_name == "sched_switch") {
		class process *old_proc = nullptr;
		class process *new_proc  = nullptr;
		std::string next_comm;
		int next_pid;
		int prev_pid;

		field = tep_find_any_field(event, "next_comm");
		if (!field || !(field->flags & TEP_FIELD_IS_STRING))
			return; /* ?? */

		next_comm = get_tep_field_str(trace, event, field);

		ret = tep_get_field_val(nullptr, event, "next_pid", &rec, &val, 0);
		if (ret < 0)
			return;
		next_pid = (int)val;

		ret = tep_get_field_val(nullptr, event, "prev_pid", &rec, &val, 0);
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

		if (old_proc && old_proc->name() != "process")
			old_proc = nullptr;

		/* retire the old process */

		if (old_proc) {
			old_proc->deschedule_thread(time, prev_pid);
			old_proc->waker = nullptr;
		}

		if (consumer_depth(cpu))
			pop_consumer(cpu);

		push_consumer(cpu, new_proc);

		/* start new process */
		new_proc->schedule_thread(time, next_pid);

		if (!std::string_view(next_comm).starts_with("migration/") && !std::string_view(next_comm).starts_with("kworker/") && !std::string_view(next_comm).starts_with("kondemand/")) {
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
		new_proc->waker = nullptr;
	}
	else if (event_name == "sched_wakeup") {
		class power_consumer *from = nullptr;
		class process *dest_proc = nullptr;
		class process *from_proc = nullptr;
		std::string comm;
		int flags;
		int pid;

		ret = tep_get_common_field_val(nullptr, event, "common_flags", &rec, &val, 0);
		if (ret < 0)
			return;
		flags = (int)val;

		if ( (flags & TRACE_FLAG_HARDIRQ) || (flags & TRACE_FLAG_SOFTIRQ)) {
			class timer *timer;
			timer = (class timer *) current_consumer(cpu);
			if (timer && timer->name() == "timer") {
				if (timer->handler != "delayed_work_timer_fn" &&
				    timer->handler != "hrtimer_wakeup" &&
				    timer->handler != "it_real_fn")
					from = timer;
			}
			/* woken from interrupt */
			/* TODO: find the current irq handler and set "from" to that */
		} else {
			from = current_consumer(cpu);
		}


		field = tep_find_any_field(event, "comm");

		if (!field || !(field->flags & TEP_FIELD_IS_STRING))
 			return;

		comm = get_tep_field_str(trace, event, field);

		ret = tep_get_field_val(nullptr, event, "pid", &rec, &val, 0);
		if (ret < 0)
			return;
		pid = (int)val;

		dest_proc = find_create_process(comm, pid);

		if (from && from->name() != "process"){
			/* not a process doing the wakeup */
			from = nullptr;
			from_proc = nullptr;
		} else {
			from_proc = (class process *) from;
		}

		if (from_proc && (dest_proc->running == 0) && (dest_proc->waker == nullptr) && (pid != 0) && !dont_blame_me(from_proc->comm))
			dest_proc->waker = from;
		if (from)
			dest_proc->last_waker = from;

		/* Account processes that wake up X specially */
		if (from && dest_proc && comm_is_xorg(dest_proc->comm))
			from->xwakes ++ ;

	}
	else if (event_name == "irq_handler_entry") {
		class interrupt *irq = nullptr;
		std::string handler;
		int nr;

		field = tep_find_any_field(event, "name");
		if (!field || !(field->flags & TEP_FIELD_IS_STRING))
			return; /* ?? */

		handler = get_tep_field_str(trace, event, field);

		ret = tep_get_field_val(nullptr, event, "irq", &rec, &val, 0);
		if (ret < 0)
			return;
		nr = (int)val;

		irq = find_create_interrupt(handler, nr, cpu);


		push_consumer(cpu, irq);

		irq->start_interrupt(time);

		if (irq->handler.find("timer") == std::string::npos)
			change_blame(cpu, irq, LEVEL_HARDIRQ);

	}

	else if (event_name == "irq_handler_exit") {
		class interrupt *irq = nullptr;
		uint64_t t;

		/* find interrupt (top of stack) */
		irq = (class interrupt *)current_consumer(cpu);
		if (!irq || irq->name() != "interrupt")
			return;
		pop_consumer(cpu);
		/* retire interrupt */
		t = irq->end_interrupt(time);
		consumer_child_time(cpu, t);
	}

	else if (event_name == "softirq_entry") {
		class interrupt *irq = nullptr;
		std::string handler;
		int vec;

		ret = tep_get_field_val(nullptr, event, "vec", &rec, &val, 0);
                if (ret < 0) {
                        fprintf(stderr, "softirq_entry event returned no vector number?\n");
                        return;
                }
		vec = (int)val;

		if (vec >= 0 && vec < (int)softirqs.size())
			handler = softirqs[vec];

		if (handler.empty())
			return;

		irq = find_create_interrupt(handler, vec, cpu);

		push_consumer(cpu, irq);

		irq->start_interrupt(time);
		change_blame(cpu, irq, LEVEL_SOFTIRQ);
	}
	else if (event_name == "softirq_exit") {
		class interrupt *irq = nullptr;
		uint64_t t;

		irq = (class interrupt *) current_consumer(cpu);
		if (!irq  || irq->name() != "interrupt")
			return;
		pop_consumer(cpu);
		/* pop irq */
		t = irq->end_interrupt(time);
		consumer_child_time(cpu, t);
	}
	else if (event_name == "timer_expire_entry") {
		class timer *timer = nullptr;
		uint64_t function;
		uint64_t tmr;

		ret = tep_get_field_val(nullptr, event, "function", &rec, &val, 0);
		if (ret < 0) {
			fprintf(stderr, "timer_expire_entry event returned no function value?\n");
			return;
		}
		function = (uint64_t)val;

		timer = find_create_timer(function);

		if (timer->is_deferred())
			return;

		ret = tep_get_field_val(nullptr, event, "timer", &rec, &val, 0);
		if (ret < 0) {
			fprintf(stderr, "timer_expire_entry event returned no timer?\n");
			return;
		}
		tmr = (uint64_t)val;

		push_consumer(cpu, timer);
		timer->fire(time, tmr);

		if (timer->handler != "delayed_work_timer_fn")
			change_blame(cpu, timer, LEVEL_TIMER);
	}
	else if (event_name == "timer_expire_exit") {
		class timer *timer = nullptr;
		uint64_t tmr;
		uint64_t t;

		ret = tep_get_field_val(nullptr, event, "timer", &rec, &val, 0);
		if (ret < 0)
			return;
		tmr = (uint64_t)val;

		timer = (class timer *) current_consumer(cpu);
		if (!timer || timer->name() != "timer") {
			return;
		}
		pop_consumer(cpu);
		t = timer->done(time, tmr);
		if (t == ~0ULL) {
			timer->fire(first_stamp, tmr);
			t = timer->done(time, tmr);
		}
		consumer_child_time(cpu, t);
	}
	else if (event_name == "hrtimer_expire_entry") {
		class timer *timer = nullptr;
		uint64_t function;
		uint64_t tmr;

		ret = tep_get_field_val(nullptr, event, "function", &rec, &val, 0);
		if (ret < 0)
			return;
		function = (uint64_t)val;

		timer = find_create_timer(function);

		ret = tep_get_field_val(nullptr, event, "hrtimer", &rec, &val, 0);
		if (ret < 0)
			return;
		tmr = (uint64_t)val;

		push_consumer(cpu, timer);
		timer->fire(time, tmr);

		if (timer->handler != "delayed_work_timer_fn")
			change_blame(cpu, timer, LEVEL_TIMER);
	}
	else if (event_name == "hrtimer_expire_exit") {
		class timer *timer = nullptr;
		uint64_t tmr;
		uint64_t t;

		timer = (class timer *) current_consumer(cpu);
		if (!timer || timer->name() != "timer") {
			return;
		}

		ret = tep_get_field_val(nullptr, event, "hrtimer", &rec, &val, 0);
		if (ret < 0)
			return;
		tmr = (uint64_t)val;

		pop_consumer(cpu);
		t = timer->done(time, tmr);
		if (t == ~0ULL) {
			timer->fire(first_stamp, tmr);
			t = timer->done(time, tmr);
		}
		consumer_child_time(cpu, t);
	}
	else if (event_name == "workqueue_execute_start") {
		class work *work = nullptr;
		uint64_t function;
		uint64_t wk;

		ret = tep_get_field_val(nullptr, event, "function", &rec, &val, 0);
		if (ret < 0)
			return;
		function = (uint64_t)val;

		ret = tep_get_field_val(nullptr, event, "work", &rec, &val, 0);
		if (ret < 0)
			return;
		wk = (uint64_t)val;

		work = find_create_work(function);


		push_consumer(cpu, work);
		work->fire(time, wk);


		if (work->handler != "do_dbs_timer" && work->handler != "vmstat_update")
			change_blame(cpu, work, LEVEL_WORK);
	}
	else if (event_name == "workqueue_execute_end") {
		class work *work = nullptr;
		uint64_t t;
		uint64_t wk;

		ret = tep_get_field_val(nullptr, event, "work", &rec, &val, 0);
		if (ret < 0)
			return;
		wk = (uint64_t)val;

		work = (class work *) current_consumer(cpu);
		if (!work || work->name() != "work") {
			return;
		}
		pop_consumer(cpu);
		t = work->done(time, wk);
		if (t == ~0ULL) {
			work->fire(first_stamp, wk);
			t = work->done(time, wk);
		}
		consumer_child_time(cpu, t);
	}
	else if (event_name == "cpu_idle") {
		ret = tep_get_field_val(nullptr, event, "state", &rec, &val, 0);
		if (ret < 0)
			return;
		if (val == (unsigned int)-1)
			consume_blame(cpu);
		else
			set_wakeup_pending(cpu);
	}
	else if (event_name == "power_start") {
		set_wakeup_pending(cpu);
	}
	else if (event_name == "power_end") {
		consume_blame(cpu);
	}
	else if (event_name == "i915_gem_ring_dispatch"
	 || event_name == "i915_gem_request_submit") {
		/* any kernel contains only one of the these tracepoints,
		 * the latter one got replaced by the former one */
		class power_consumer *consumer = nullptr;
		int flags;

		ret = tep_get_common_field_val(nullptr, event, "common_flags", &rec, &val, 0);
		if (ret < 0)
			return;
		flags = (int)val;

		consumer = current_consumer(cpu);
		/* currently we don't count graphic requests submitted from irq contect */
		if ( (flags & TRACE_FLAG_HARDIRQ) || (flags & TRACE_FLAG_SOFTIRQ)) {
			consumer = nullptr;
		}


		/* if we are X, and someone just woke us, account the GPU op to the guy waking us */
		if (consumer && consumer->name() == "process") {
			class process *proc = nullptr;
			proc = (class process *) consumer;
			if (comm_is_xorg(proc->comm) && proc->last_waker) {
				consumer = proc->last_waker;
			}
		}



		if (consumer) {
			consumer->gpu_ops++;
		}
	}
	else if (event_name == "writeback_inode_dirty") {
		class power_consumer *consumer = nullptr;
		int dev;

		consumer = current_consumer(cpu);

		ret = tep_get_field_val(nullptr, event, "dev", &rec, &val, 0);
		if (ret < 0)

			return;
		dev = (int)val;

		if (consumer && consumer->name() == "process" && dev > 0) {

			consumer->disk_hits++;

			/* if the previous inode dirty was > 1 second ago, it becomes a hard hit */
			if ((time - prev_disk_time) > 1000000000)
				consumer->hard_disk_hits++;

			prev_disk_time = time;
		}
	}
}

void start_process_measurement(void)
{
	if (!perf_events) {
		perf_events = new perf_process_bundle();
		perf_events->add_event("sched","sched_switch");
		perf_events->add_event("sched","sched_wakeup");
		perf_events->add_event("irq","irq_handler_entry");
		perf_events->add_event("irq","irq_handler_exit");
		perf_events->add_event("irq","softirq_entry");
		perf_events->add_event("irq","softirq_exit");
		perf_events->add_event("timer","timer_expire_entry");
		perf_events->add_event("timer","timer_expire_exit");
		perf_events->add_event("timer","hrtimer_expire_entry");
		perf_events->add_event("timer","hrtimer_expire_exit");
		if (!perf_events->add_event("power","cpu_idle")){
			perf_events->add_event("power","power_start");
			perf_events->add_event("power","power_end");
		}
		perf_events->add_event("workqueue","workqueue_execute_start");
		perf_events->add_event("workqueue","workqueue_execute_end");
		perf_events->add_event("i915","i915_gem_ring_dispatch");
		perf_events->add_event("i915","i915_gem_request_submit");
		perf_events->add_event("writeback","writeback_inode_dirty");
	}

	first_stamp = ~0ULL;
	last_stamp = 0;
	prev_disk_time = 0;
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
	if (measurement_time < 0.00001)
		return 0.0;
	double total = 0;
	unsigned int i;
	for (i = 0; i < all_power.size() ; i++)
		total += all_power[i]->wake_ups;

	return total / measurement_time;
}

double total_gpu_ops(void)
{
	if (measurement_time < 0.00001)
		return 0.0;
	double total = 0;
	unsigned int i;
	for (i = 0; i < all_power.size() ; i++)
		total += all_power[i]->gpu_ops;

	return total / measurement_time;
}

double total_disk_hits(void)
{
	if (measurement_time < 0.00001)
		return 0.0;
	double total = 0;
	unsigned int i;
	for (i = 0; i < all_power.size() ; i++)
		total += all_power[i]->disk_hits;

	return total / measurement_time;
}


double total_hard_disk_hits(void)
{
	if (measurement_time < 0.00001)
		return 0.0;
	double total = 0;
	unsigned int i;
	for (i = 0; i < all_power.size() ; i++)
		total += all_power[i]->hard_disk_hits;

	return total / measurement_time;
}

double total_xwakes(void)
{
	if (measurement_time < 0.00001)
		return 0.0;
	double total = 0;
	unsigned int i;
	for (i = 0; i < all_power.size() ; i++)
		total += all_power[i]->xwakes;

	return total / measurement_time;
}

void process_update_display(void)
{
	unsigned int i;
	WINDOW *win;
	double pw;
	double joules;
	int tl;
	int tlt;
	int tlr;

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
				all_parameters.guessed_power, global_power(), sum);
#endif

	pw = global_power();
	joules = global_joules();
	tl = global_time_left() / 60;
	tlt = (tl /60);
	tlr = tl % 60;

	if (pw > 0.0001) {
		wprintw(win, _("The battery reports a discharge rate of %sW\n"),
				fmt_prefix(pw).c_str());
		wprintw(win, _("The energy consumed was %sJ\n"),
				fmt_prefix(joules).c_str());
		need_linebreak = 1;
	}
	if (tl > 0 && pw > 0.0001) {
		wprintw(win, _("The estimated remaining time is %i hours, %i minutes\n"), tlt, tlr);
		need_linebreak = 1;
	}

	if (need_linebreak)
		wprintw(win, "\n");


	wprintw(win, "%s: %3.1f %s,  %3.1f %s, %3.1f %s %3.1f%% %s\n\n",_("Summary"), total_wakeups(), _("wakeups/second"), total_gpu_ops(), _("GPU ops/seconds"), total_disk_hits(), _("VFS ops/sec and"), total_cpu_time()*100, _("CPU use"));


	if (show_power)
		wprintw(win, "%s              %s       %s    %s       %s\n", _("Power est."), _("Usage"), _("Events/s"), _("Category"), _("Description"));
	else
		wprintw(win, "                %s       %s    %s       %s\n", _("Usage"), _("Events/s"), _("Category"), _("Description"));

	for (i = 0; i < all_power.size(); i++) {
		std::string power;
		std::string name;
		std::string usage;
		std::string events;
		power = format_watts(all_power[i]->Witts(), 10);

		if (!show_power)
			power = "          ";
		name = all_power[i]->type();

		if (all_power[i]->events() == 0 && all_power[i]->usage() == 0 && all_power[i]->Witts() == 0)
			break;

		if (!all_power[i]->usage_units().empty()) {
			if (all_power[i]->usage() < 1000)
				usage = std::format("{:5.1f}{}", all_power[i]->usage(), all_power[i]->usage_units());
			else
				usage = std::format("{:5d}{}", (int)all_power[i]->usage(), all_power[i]->usage_units());
		}


		events = std::format("{:5.1f}", all_power[i]->events());
		if (!all_power[i]->show_events())
			events = "";
		else if (all_power[i]->events() <= 0.3)
			events = std::format("{:5.2f}", all_power[i]->events());

		align_string(name, 14, 20);
		align_string(usage, 14, 20);
		align_string(events, 12, 20);

		wprintw(win, "%s  %s %s %s %s\n", 
			power.c_str(), 
			usage.c_str(), 
			events.c_str(), 
			name.c_str(), 
			pretty_print(all_power[i]->description()).c_str());
	}
}

void report_process_update_display(void)
{
	unsigned int i;
	unsigned int total;
	int show_power, cols, rows, idx;

	/* div attr css_class and css_id */
	tag_attr div_attr;
	init_div(&div_attr, "clear_block", "software");

	/* Set Table attributes, rows, and cols */
	cols=7;
	sort(all_power.begin(), all_power.end(), power_cpu_sort);
	show_power = global_power_valid();
	if (show_power)
		cols=8;

	idx=cols;

	total = all_power.size();
	if (total > 100)
		total = 100;

	rows=total+1;
	table_attributes std_table_css;
	init_nowarp_table_attr(&std_table_css, rows, cols);


	/* Set Title attributes */
	tag_attr title_attr;
	init_title_attr(&title_attr);

	/* Set array of data in row Major order */
	std::vector<std::string> software_data(cols * rows);
	software_data[0]=__("Usage");
	software_data[1]=__("Wakeups/s");
	software_data[2]=__("GPU ops/s");
	software_data[3]=__("Disk IO/s");
	software_data[4]=__("GFX Wakeups/s");
	software_data[5]=__("Category");
	software_data[6]=__("Description");

	if (show_power)
		software_data[7]=__("PW Estimate");


	for (i = 0; i < total; i++) {
		std::string power, name, usage, wakes, gpus, disks, xwakes;
		power = format_watts(all_power[i]->Witts(), 10);

		if (!show_power)
			power = "          ";
		name = all_power[i]->type();

		if (name == "Device")
			continue;

		if (all_power[i]->events() == 0 && all_power[i]->usage() == 0
				&& all_power[i]->Witts() == 0)
			break;

		if (!all_power[i]->usage_units().empty()) {
			if (all_power[i]->usage() < 1000)
				usage = std::format("{:5.1f}{}", all_power[i]->usage(), all_power[i]->usage_units());
			else
				usage = std::format("{:5d}{}", (int)all_power[i]->usage(), all_power[i]->usage_units());
		}

		wakes = "  0.0";
		gpus = "  0.0";
		disks = "  0.0 (  0.0)";
		xwakes = "  0.0";
		if (measurement_time >= 0.00001) {
			wakes = std::format("{:5.1f}", all_power[i]->wake_ups / measurement_time);
			if (all_power[i]->wake_ups / measurement_time <= 0.3)
				wakes = std::format("{:5.2f}", all_power[i]->wake_ups / measurement_time);

			gpus = std::format("{:5.1f}", all_power[i]->gpu_ops / measurement_time);

			disks = std::format("{:5.1f} ({:5.1f})",
					all_power[i]->hard_disk_hits / measurement_time,
					all_power[i]->disk_hits / measurement_time);

			xwakes = std::format("{:5.1f}", all_power[i]->xwakes / measurement_time);
		}

		if (!all_power[i]->show_events()) {
			wakes = "";
			gpus = "";
			disks = "";
		}

		if (all_power[i]->gpu_ops == 0)
			gpus = "";
		if (all_power[i]->wake_ups == 0)
			wakes = "";
		if (all_power[i]->disk_hits == 0)
			disks = "";
		if (all_power[i]->xwakes == 0)
			xwakes = "";

		software_data[idx]=usage;
		idx+=1;

		software_data[idx]=wakes;
		idx+=1;

		software_data[idx]=gpus;
		idx+=1;

		software_data[idx]=disks;
		idx+=1;

		software_data[idx]=xwakes;
		idx+=1;

		software_data[idx]=name;
		idx+=1;

		software_data[idx]=pretty_print(all_power[i]->description());
		idx+=1;
		if (show_power) {
			software_data[idx]=power;
			idx+=1;
		}
	}

	/* Report Output */
	report.add_div(&div_attr);
	report.add_title(&title_attr, __("Overview of Software Power Consumers"));
	report.add_table(software_data, &std_table_css);
        report.end_div();
}

void report_summary(void)
{
	unsigned int i;
	unsigned int total;
	int show_power;
	int rows, cols, idx;

	sort(all_power.begin(), all_power.end(), power_cpu_sort);
	show_power = global_power_valid();

	/* div attr css_class and css_id */
	tag_attr div_attr;
	init_div(&div_attr, "clear_block", "summary");


	/* Set table attributes, rows, and cols */
	cols=4;
	if (show_power)
		cols=5;
	idx=cols;
	total = all_power.size();
	if (total > 10)
		total = 10;
	rows=total+1;
	table_attributes std_table_css;
	init_std_table_attr(&std_table_css, rows, cols);

	/* Set title attributes */
	tag_attr title_attr;
	init_title_attr(&title_attr);

	/* Set array for summary */
	int summary_size = 12;
	std::vector<std::string> summary(summary_size);
	summary[0]=__("Target:");
	summary[1]=__("1 units/s");
	summary[2]=__("System: ");
	summary[3]=std::format("{:5.1f} {}", total_wakeups(), __(" wakeup/s"));
	summary[4]=__("CPU: ");
	summary[5]=std::format("{:5.1f}% {}", total_cpu_time()*100, __(" usage"));
	summary[6]=__("GPU:");
	summary[7]=std::format("{:5.1f} {}", total_gpu_ops(), __(" ops/s"));
	summary[8]=__("GFX:");
	summary[9]=std::format("{:5.1f} {}", total_xwakes(), __(" wakeups/s"));
	summary[10]=__("VFS:");
	summary[11]=std::format("{:5.1f} {}", total_disk_hits(), __(" ops/s"));

	/* Set array of data in row Major order */
	std::vector<std::string> summary_data(cols * (rows + 1));
	summary_data[0]=__("Usage");
	summary_data[1]=__("Events/s");
	summary_data[2]=__("Category");
	summary_data[3]=__("Description");
	if (show_power)
		summary_data[4]=__("PW Estimate");

	for (i = 0; i < all_power.size(); i++) {
		std::string power, name, usage, events;
		power = format_watts(all_power[i]->Witts(), 10);

		if (!show_power)
			power = "          ";
		name = all_power[i]->type();

		if (i > total)
			break;

		if (all_power[i]->events() == 0 && all_power[i]->usage() == 0 &&
				all_power[i]->Witts() == 0)
			break;

		if (!all_power[i]->usage_units().empty()) {
			if (all_power[i]->usage() < 1000)
				usage = std::format("{:5.1f}{}", all_power[i]->usage_summary(),
					all_power[i]->usage_units_summary());
			else
				usage = std::format("{:5d}{}", (int)all_power[i]->usage_summary(),
					all_power[i]->usage_units_summary());
		}
		
		events = std::format("{:5.1f}", all_power[i]->events());
		if (!all_power[i]->show_events())
			events = "";
		else if (all_power[i]->events() <= 0.3)
			events = std::format("{:5.2f}", all_power[i]->events());

		summary_data[idx]=usage;
		idx+=1;

		summary_data[idx]=events;
		idx+=1;

		summary_data[idx]=name;
		idx+=1;

		summary_data[idx]=pretty_print(all_power[i]->description());
		idx+=1;

		if (show_power){
			summary_data[idx]=power;
			idx+=1;
		}
	}

	/* Report Summary for all */
	report.add_summary_list(summary);
	report.add_div(&div_attr);
	report.add_title(&title_attr, __("Top 10 Power Consumers"));
	report.add_table(summary_data, &std_table_css);
	report.end_div();
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
	cpu_blame.resize(0, nullptr);
	cpu_blame.resize(get_max_cpu()+1, nullptr);



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
	perf_events = nullptr;
}

