/* Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
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
 * Common part of report_formatter_csv and report_formatter_html.
 * Written by Igor Zhbanov <i.zhbanov@samsung.com>
 * 2012.10 */

#define _BSD_SOURCE

/* Uncomment to disable asserts */
/*#define NDEBUG*/

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "report-formatter-base.h"

/* ************************************************************************ */

const char *
report_formatter_string_base::get_result()
{
	return result.c_str();
}

/* ************************************************************************ */

void
report_formatter_string_base::clear_result()
{
	result.clear();
}

/* ************************************************************************ */

void
report_formatter_string_base::add(const char *str)
{
	assert(str);

	result += escape_string(str);
}

/* ************************************************************************ */

void
report_formatter_string_base::add_exact(const char *str)
{
	assert(str);

	result += std::string(str);
}

/* ************************************************************************ */

#define LINE_SIZE 8192

void
report_formatter_string_base::addv(const char *fmt, va_list ap)
{
	char str[LINE_SIZE];

	assert(fmt);

	vsnprintf(str, sizeof(str), fmt, ap);
	add(str);
}

/* ************************************************************************ */

void
report_formatter_string_base::addf(const char *fmt, ...)
{
	va_list ap;

	assert(fmt);

	va_start(ap, fmt);
	addv(fmt, ap);
	va_end(ap);
}

/* ************************************************************************ */

void
report_formatter_string_base::addf_exact(const char *fmt, ...)
{
	char str[LINE_SIZE];
	va_list ap;

	assert(fmt);

	va_start(ap, fmt);
	vsnprintf(str, sizeof(str), fmt, ap);
	add_exact(str);
	va_end(ap);
}
