#ifndef _INCLUDE_GUARD_TIMER_H
#define _INCLUDE_GUARD_TIMER_H

#include <stdint.h>

#include "powerconsumer.h"

class timer : public power_consumer {
	uint64_t	running_since;
	char desc[256];
public:
	char 		handler[32];
	int		raw_count;

	timer(unsigned long timer_func);

	void fire(uint64_t time);
	uint64_t done(uint64_t time);

	virtual double Witts(void);
	virtual const char * description(void);
	virtual const char * name(void) { return "timer"; };

};

class timer_list {
public:
	uint64_t	timer_address;
	uint64_t	timer_func;
};


extern void all_timers_to_all_power(void);

#endif