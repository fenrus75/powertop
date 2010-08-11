#ifndef _INCLUDE_GUARD_PERF_BUNDLE_H_
#define _INCLUDE_GUARD_PERF_BUNDLE_H_

#include <iostream>
#include <vector>


using namespace std;

#include "perf.h"
class perf_event;

class  perf_bundle {
protected:
	vector<class perf_event *> events;
public:
	vector<void *> records;

	void add_event(const char *event_name);

	void start(void);
	void stop(void);
	void clear(void);

	void process(void);

	virtual void handle_trace_point(int type, void *trace);
};


#endif