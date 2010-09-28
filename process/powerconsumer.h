#ifndef __INCLUDE_GUARD_POWER_CONSUMER_
#define __INCLUDE_GUARD_POWER_CONSUMER_

#include <stdint.h>
#include <vector>
#include <algorithm>

using namespace std;

extern double measurement_time;

class power_consumer;

class power_consumer {

public:
	uint64_t	accumulated_runtime;
	uint64_t	child_runtime;
	int	 	disk_hits;
	int		wake_ups;
	int		gpu_ops;
	class power_consumer *waker;
	class power_consumer *last_waker;

	power_consumer(void);

	virtual double Witts(void);
	virtual const char * description(void) { return ""; };

	virtual const char * name(void) { return "abstract"; };
	virtual const char * type(void) { return "abstract"; };

	virtual double usage(void) { return (accumulated_runtime - child_runtime) / 1000000.0 / measurement_time;};
	virtual const char * usage_units(void) {return "msec";};
	virtual double events(void) { return  (wake_ups + gpu_ops) / measurement_time;};
	virtual int show_events(void) { return 1; };
};

extern vector <class power_consumer *> all_power;

extern double total_wakeups(void);
extern double total_cpu_time(void);
extern double total_gpu_ops(void);



#endif