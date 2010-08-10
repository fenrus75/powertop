#ifndef _INCLUDE_GUARD_PERF_H_
#define _INCLUDE_GUARD_PERF_H_

#include <iostream>
#include <vector>

using namespace std;

class  perf_bundle {
protected:
	vector<class perf_event *> events;
public:
	void add_event(char *event_name);

	void start(void);
	void stop(void);
	void clear(void);

	void process(void);

};


#endif