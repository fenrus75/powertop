#include "process.h"


#include "../perf/perf_bundle.h"


class perf_process_bundle: public perf_bundle
{
        virtual void handle_trace_point(int type, void *trace, int cpu, uint64_t time);
};



void perf_process_bundle::handle_trace_point(int type, void *trace, int cpu, uint64_t time)
{
}





void start_process_measurement(void)
{
}

void end_process_measurement(void)
{
}


void process_process_data(void)
{
}

