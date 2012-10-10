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
 * CSV report generator.
 * Written by Igor Zhbanov <i.zhbanov@samsung.com>
 * 2012.10 */

#ifndef _REPORT_FORMATTER_CSV_H_
#define _REPORT_FORMATTER_CSV_H_

#include <string>

#include "report-formatter-base.h"

/* Offices like semicolon separated values instead of comma */
#define REPORT_CSV_DELIMITER ';'

/* "a,b,c" vs "a, b, c" */
/*#define REPORT_CSV_ADD_SPACE*/

/* Whether to escape with quotes empty cell values */
/*#define REPORT_CSV_ESCAPE_EMPTY*/

/* Whether to escape with quotes empty cell values with spaces */
/*#define REPORT_CSV_SPACE_NEED_QUOTES*/

/* ************************************************************************ */

class report_formatter_csv: public report_formatter_string_base
{
public:
	report_formatter_csv();

	void finish_report();

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

private:
	void add_doc_header();

	void add_quotes();

	std::string escape_string(const char *str);

	bool csv_need_quotes;
	size_t table_cell_number, text_start;
};

#endif /* _REPORT_FORMATTER_CSV_H_ */
