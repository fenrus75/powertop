/*
 * Copyright 2015, Intel Corporation
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
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 *	Daniel Leung <daniel.leung@linux.intel.com>
 */

#ifndef _INCLUDE_GUARD_I2C_TUNE_H
#define _INCLUDE_GUARD_I2C_TUNE_H

#include <vector>
#include <limits.h>

#include "tunable.h"

using namespace std;

class i2c_tunable : public tunable {
	char i2c_path[PATH_MAX];
public:
	i2c_tunable(const char *path, const char *name, bool is_adapter);

	virtual int good_bad(void);

	virtual void toggle(void);

	virtual const char *toggle_script(void);

};

extern void add_i2c_tunables(void);


#endif
