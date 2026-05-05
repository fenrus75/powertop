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

/* Uncomment to disable asserts */
/*#define NDEBUG*/

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <format>

#include "report-formatter-csv.h"
#include "report-data-html.h"


/* ************************************************************************ */
report_formatter_csv::report_formatter_csv()
{
	/* Do nothing special  */
}

/* ************************************************************************ */
void
report_formatter_csv::finish_report()
{
	/* Do nothing special */
}


std::string
report_formatter_csv::escape_string(const std::string &str)
{
	std::string res;

	for (size_t i = 0; i < str.length(); i++) {
		switch (str[i]) {
			case '"':
				res += '"';
				[[fallthrough]];
#ifdef REPORT_CSV_SPACE_NEED_QUOTES
			case ' ':
#endif /* REPORT_CSV_SPACE_NEED_QUOTES */
			case '\n':
			case REPORT_CSV_DELIMITER:
				csv_need_quotes = true;
				break;
		}

		res += str[i];
	}

	return res;
}




/* Report Style */
void
report_formatter_csv::add_header()
{
	add_exact("____________________________________________________________________\n");
}

void
report_formatter_csv::end_header()
{
	/* Do nothing */
}

void
report_formatter_csv::add_logo()
{
	add_exact("\t\t\tP o w e r T O P\n");
}


void report_formatter_csv::add_div([[maybe_unused]] struct tag_attr *div_attr)

{
	add_exact("\n");
}

void
report_formatter_csv::end_div()
{
	/*Do nothing*/
}

void report_formatter_csv::add_title([[maybe_unused]] struct tag_attr *title_att, const std::string &title)

{
	add_exact("____________________________________________________________________\n");
	add_exact(std::format(" *  *  *   {}   *  *  *\n", title));
}

void
report_formatter_csv::add_navigation()
{
	/* No nav in csv - thinking on table of contents */
}

void
report_formatter_csv::add_summary_list(const std::vector<std::string> &list)
{
	add_exact("\n");
	for (size_t i = 0; i + 1 < list.size(); i += 2) {
		add_exact(std::format("{} {}", list[i], list[i + 1]));
		if (i + 2 < list.size())
			add_exact(";");
	}
	add_exact("\n");
}


void
report_formatter_csv::add_table(const std::vector<std::string> &system_data, struct table_attributes* tb_attr)
{
	int i, j;
	int offset=0;
	std::string tmp_str="";
	add_exact("\n");
	for (i=0; i < tb_attr->rows; i++){
		std::string row_buf = "";
		bool all_empty = true;
		for (j=0; j < tb_attr->cols; j++){
			offset = i * (tb_attr->cols) + j;

			if (offset >= (int)system_data.size())
				break;

			tmp_str=system_data[offset];

			if (tmp_str != "&nbsp;") {
				row_buf += system_data[offset];
				all_empty = false;
			}

			if (j < (tb_attr->cols - 1))
				row_buf += ";";
		}
		if (!all_empty) {
			add_exact(row_buf);
			add_exact("\n");
		}
	}
}
