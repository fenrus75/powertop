#ifndef __INCLUDE_GUARD_POWER_CONSUMER_
#define __INCLUDE_GUARD_POWER_CONSUMER_

class power_consumer {
	virtual double Witts(void) { return 0.0;};
	virtual const char * description(void) { return ""; };
};

#endif