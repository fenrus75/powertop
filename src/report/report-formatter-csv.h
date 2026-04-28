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

#pragma once

#include <string>
#include <vector>

#include "report-formatter-base.h"

/* Offices like semicolon separated values instead of comma */
#define REPORT_CSV_DELIMITER ','

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
	void finish_report() override;

	/* Report Style */
	void add_logo() override;
	void add_header() override;
	void end_header() override;
	void add_div(struct tag_attr *div_attr) override;
	void end_div() override;
	void add_title(struct tag_attr *title_att, const std::string &title) override;
	void add_navigation() override;
	void add_summary_list(const std::vector<std::string> &list) override;
	void add_table(const std::vector<std::string> &system_data, struct table_attributes *tb_attr) override;private:
	std::string escape_string(const std::string &str) override;
	bool csv_need_quotes = false;
};

