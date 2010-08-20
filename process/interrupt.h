#ifndef _INCLUDE_GUARD_INTERRUPT_H
#define _INCLUDE_GUARD_INTERRUPT_H

#include <stdint.h>

#include "powerconsumer.h"

class interrupt : public power_consumer {
	uint64_t	running_since;
	char		desc[256];
public:
	char 		handler[32];
	int 		number;

	int		raw_count;

	interrupt(const char *_handler, int _number);

	virtual void start_interrupt(uint64_t time);
	virtual uint64_t end_interrupt(uint64_t time);

	virtual double Witts(void);
	virtual const char * description(void);


};

extern vector <class interrupt *> all_interrupts;
extern const char* softirqs[];


extern class interrupt * find_create_interrupt(const char *_handler, int nr, int cpu);
extern void all_interrupts_to_all_power(void);




#endif