#ifndef _INCLUDE_GUARD_INTERRUPT_H
#define _INCLUDE_GUARD_INTERRUPT_H

#include <stdint.h>

#include "powerconsumer.h"

class interrupt : public power_consumer {
	uint64_t	running_since;
public:
	char 		handler[32];
	int 		number;

	uint64_t	accumulated_runtime;
	int		wake_ups;

	interrupt(const char *_handler, int _number);

	virtual void start_interrupt(uint64_t time, int from_idle = 0);
	virtual void end_interrupt(uint64_t time);
};


#endif