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
#pragma once

#include <pthread.h>
#include <atomic>
#include <mutex>
#include "measurement.h"

class extech_power_meter: public power_meter {
	int fd = 0;

	double rate = 0.0;
	void measure(void);
	double sum = 0.0;
	int samples = 0;
	std::atomic<bool> end_thread{false};
	std::mutex samples_mutex;
	pthread_t thread;
public:
	extech_power_meter(const std::string &_dev_name);
	virtual void start_measurement(void) override;
	virtual void end_measurement(void) override;
	virtual void sample(void);

	virtual double power(void) override;
	virtual double dev_capacity(void) override { return 0.0; };
};

