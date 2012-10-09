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
 * report_formatter interface.
 * Written by Igor Zhbanov <i.zhbanov@samsung.com>
 * 2012.10 */

#ifndef _REPORT_FORMATTER_H_
#define _REPORT_FORMATTER_H_

#include "report-maker.h"

class report_formatter /* Interface */
{
public:
	virtual ~report_formatter() {}

	virtual void finish_report() = 0;
	virtual const char *get_result() = 0;
	virtual void clear_result() = 0;

	virtual void add(const char *str) = 0;
	virtual void addv(const char *fmt, va_list ap) = 0;

	virtual void add_header(const char *header, int level) = 0;

	virtual void begin_section(section_type stype) = 0;
	virtual void end_section() = 0;

	virtual void begin_table(table_type ttype) = 0;
	virtual void end_table() = 0;

	virtual void begin_row(row_type rtype) = 0;
	virtual void end_row() = 0;

	virtual void begin_cell(cell_type ctype) = 0;
	virtual void end_cell() = 0;
	virtual void add_empty_cell() = 0;

	virtual void begin_paragraph() = 0;
	virtual void end_paragraph() = 0;

	/* For quad-colouring CPU tables in HTML */
	virtual void set_cpu_number(int nr) = 0;
};

#endif /* _REPORT_FORMATTER_H_ */
