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

#pragma once

#include <vector>
#include <limits.h>

#include "tunable.h"

class i2c_tunable : public tunable {
	std::string i2c_path;
public:
	i2c_tunable(const std::string &path, const std::string &name, bool is_adapter);

	virtual int good_bad(void) override;

	virtual void toggle(void) override;

};

extern void add_i2c_tunables(void);

