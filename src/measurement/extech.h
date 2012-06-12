/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */
#ifndef __INCLUDE_GUARD_EXTECH_H
#define __INCLUDE_GUARD_EXTECH_H

#include <pthread.h>
#include "measurement.h"

class extech_power_meter: public power_meter {
	char dev_name[256];
	int fd;

	double rate;
	void measure(void);
	double sum;
	int samples;
	int end_thread;
	pthread_t thread;
public:
	extech_power_meter(const char *_dev_name);
	virtual void start_measurement(void);
	virtual void end_measurement(void);
	virtual void sample(void);
	
	virtual double joules_consumed(void);
	virtual double dev_capacity(void) { return 0.0; };
};

#endif
