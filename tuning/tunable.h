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
#ifndef _INCLUDE_GUARD_TUNABLE_H
#define _INCLUDE_GUARD_TUNABLE_H

#include <vector>

#include "../lib.h"

using namespace std;

#define TUNE_GOOD    1
#define TUNE_BAD     -1
#define TUNE_UNFIXABLE -2
#define TUNE_UNKNOWN 0
#define TUNE_NEUTRAL 0

class tunable {

	char good_string[128];
	char bad_string[128];
	char neutral_string[128];
protected:
	char toggle_good[4096];
	char toggle_bad[4096];
public:
	char desc[4096];
	double score;

	tunable(void);
	tunable(const char *str, double _score, const char *good = "", const char *bad = "", const char *neutral ="");
	virtual ~tunable() {};

	virtual int good_bad(void) { return TUNE_NEUTRAL; }

	virtual char *result_string(void)
	{
		switch (good_bad()) {
		case TUNE_GOOD:
			return good_string;
		case TUNE_BAD:
		case TUNE_UNFIXABLE:
			return bad_string;
		}
		return neutral_string;
	}


	virtual const char *description(void) { return desc; };

	virtual void toggle(void) { };

	virtual const char *toggle_script(void) { return NULL; }
};

extern vector<class tunable *> all_tunables;
extern vector<class tunable *> all_untunables;

#endif
