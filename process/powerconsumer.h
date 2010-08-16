#ifndef __INCLUDE_GUARD_POWER_CONSUMER_
#define __INCLUDE_GUARD_POWER_CONSUMER_

class power_consumer {

public:
	uint64_t	accumulated_runtime;
	int	 	disk_hits;
	int		wake_ups;

	virtual double Witts(void) { return 0.0;};
	virtual const char * description(void) { return ""; };
};

#endif