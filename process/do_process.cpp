#include "process.h"

#include "stdio.h"

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

void perf_process_bundle::handle_trace_point(int type, void *trace, int cpu, uint64_t time)
{
	const char *event_name;
	struct sched_switch *sw;

	sw = (struct sched_switch *)trace;

	if (type >= (int)event_names.size())
		return;
	event_name = event_names[type];
	printf("%03i  %08llu	<---- %s \n", cpu, time, sw->prev_comm);
	printf("%03i  %08llu	                  -----> %s \n", cpu, time, sw->next_comm);
}

void start_process_measurement(void)
{
	if (!perf_events) {
		perf_events = new perf_process_bundle();
		 perf_events->add_event("sched:sched_switch");
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

