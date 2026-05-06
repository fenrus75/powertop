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
	bool need_quotes = false;

	for (size_t i = 0; i < str.length(); i++) {
		switch (str[i]) {
			case '"':
				res += '"';
				need_quotes = true;
				break;
			case '\n':
			case ',':
				need_quotes = true;
				break;
		}

		res += str[i];
	}

	if (need_quotes)
		return "\"" + res + "\"";

	return res;
}




/* Report Style */
void
report_formatter_csv::add_header()
{
}

void
report_formatter_csv::end_header()
{
}

void
report_formatter_csv::add_logo()
{
	add_exact("PowerTOP\n");
}


void report_formatter_csv::add_div([[maybe_unused]] struct tag_attr *div_attr)
{
}

void
report_formatter_csv::end_div()
{
}

void report_formatter_csv::add_title([[maybe_unused]] struct tag_attr *title_att, const std::string &title)
{
	add_exact(std::format("\n# {}\n", title));
}

void
report_formatter_csv::add_navigation()
{
	/* No nav in csv - thinking on table of contents */
}

void
report_formatter_csv::add_summary_list(const std::vector<std::string> &list)
{
	for (size_t i = 0; i + 1 < list.size(); i += 2) {
		add_exact(std::format("{},{}\n", escape_string(list[i]), escape_string(list[i + 1])));
	}
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
				row_buf += escape_string(system_data[offset]);
				all_empty = false;
			}

			if (j < (tb_attr->cols - 1))
				row_buf += ",";
		}
		if (!all_empty) {
			add_exact(row_buf);
			add_exact("\n");
		}
	}
}
