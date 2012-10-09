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
 * Null report generator.
 * Written by Igor Zhbanov <i.zhbanov@samsung.com>
 * 2012.10 */

#ifndef _REPORT_FORMATTER_NULL_H_
#define _REPORT_FORMATTER_NULL_H_

#include "report-formatter.h"

/* ************************************************************************ */

class report_formatter_null: public report_formatter
{
public:
	report_formatter_null();

	void finish_report();
	const char *get_result();
	void clear_result();

	void add(const char *str);
	void addv(const char *fmt, va_list ap);

	void add_header(const char *header, int level);

	void begin_section(section_type stype);
	void end_section();

	void begin_table(table_type ttype);
	void end_table();

	void begin_row(row_type rtype);
	void end_row();

	void begin_cell(cell_type ctype);
	void end_cell();
	void add_empty_cell();

	void begin_paragraph();
	void end_paragraph();

	void set_cpu_number(int nr);
};

#endif /* _REPORT_FORMATTER_NULL_H_ */
