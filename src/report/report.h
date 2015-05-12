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
#ifndef __INCLUDE_GUARD_REPORT_H_
#define __INCLUDE_GUARD_REPORT_H_

#include <string>
#include <stdio.h>
#include <limits.h>

#include "report-maker.h"

using namespace std;

struct reportstream {
	FILE *report_file;
	char filename[PATH_MAX];
};

extern report_type reporttype;
extern report_maker report;
extern struct reportstream reportout;
extern void init_report_output(char *filename_str, int iterations);
extern void finish_report_output(void);

#endif
