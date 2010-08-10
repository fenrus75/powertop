#ifndef _INCLUDE_GUARD_PERF_H_
#define _INCLUDE_GUARD_PERF_H_

#include <iostream>

using namespace std;

class  perf_event {
protected:
	int perf_fd;
	void * perf_mmap;
	void * data_mmap;
	struct perf_event_mmap_page *pc;



	int bufsize;
	char *name;
	int cpu;
	unsigned int trace_type;
	void create_perf_event(char *eventname, int cpu);

public:
	perf_event(void);
	perf_event(const char *event_name, int cpu = 0, int buffer_size = 128);


	void set_event_name(const char *event_name);
	void set_cpu(int cpu);

	void start(void);
	void stop(void);
	void clear(void);

	void process(void *cookie);

	virtual void handle_event(struct perf_event_header *header, void *cookie) { };
};


#endif