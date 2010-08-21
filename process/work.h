#ifndef _INCLUDE_GUARD_WORK_H
#define _INCLUDE_GUARD_WORK_H

#include <stdint.h>

#include "powerconsumer.h"

class work : public power_consumer {
	char desc[256];
public:
	char 		handler[32];
	int		raw_count;

	work(unsigned long work_func);

	void fire(uint64_t time, uint64_t work_struct);
	uint64_t done(uint64_t time, uint64_t work_struct);

	virtual double Witts(void);
	virtual const char * description(void);
	virtual const char * name(void) { return "work"; };

};


extern void all_work_to_all_power(void);
extern class work * find_create_work(uint64_t func);

#endif