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
#define REPORT_CSV_DELIMITER ','

/* "a,b,c" vs "a, b, c" */
/*#define REPORT_CSV_ADD_SPACE*/

/* Whether to escape with quotes empty cell values */
/*#define REPORT_CSV_ESCAPE_EMPTY*/

/* Whether to escape with quotes empty cell values with spaces */
/*#define REPORT_CSV_SPACE_NEED_QUOTES*/

using namespace std;

/* ************************************************************************ */

class report_formatter_csv: public report_formatter_string_base
{
public:
	report_formatter_csv();
	void finish_report();

	/* Report Style */
	void add_logo();
	void add_header();
	void end_header();
	void add_div(struct tag_attr *div_attr);
	void end_div();
	void add_title(struct tag_attr *title_att, const char *title);
	void add_navigation();
	void add_summary_list(string *list, int size);
	void add_table(string *system_data, struct table_attributes *tb_attr);

private:
	void add_quotes();
	string escape_string(const char *str);
	bool csv_need_quotes;
	size_t text_start;
};

#endif /* _REPORT_FORMATTER_CSV_H_ */
