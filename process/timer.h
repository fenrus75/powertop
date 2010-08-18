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

	void fire(uint64_t time, int from_idle);
	void done(uint64_t time);

	virtual double Witts(void);
	virtual const char * description(void);

};

class timer_list {
public:
	uint64_t	timer_address;
	uint64_t	timer_func;
};


extern void timer_arm(uint64_t timer_address, uint64_t timer_func);
extern void timer_fire(uint64_t timer_address, uint64_t handler, uint64_t time, int from_idle = 0);
extern void timer_done(uint64_t timer_address, uint64_t time);
extern void timer_cancel(uint64_t timer_address);

extern void all_timers_to_all_power(void);

#endif