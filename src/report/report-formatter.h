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

#pragma once

#include <vector>
#include "report-maker.h"
#include "../lib.h"

class report_formatter
{
public:
	virtual ~report_formatter() {}

	virtual void finish_report() {}
	virtual std::string get_result() {return "Basic report_formatter::get_result() call\n";}
	virtual void clear_result() {}

	virtual void add(const std::string &[[maybe_unused]] str) {}

	/* *** Report Style *** */
	virtual void add_logo() {}
	virtual void add_header() {}
	virtual void end_header() {}
	virtual void add_div(struct tag_attr *[[maybe_unused]] div_attr) {}
	virtual void end_div() {}
	virtual void add_title(struct tag_attr *[[maybe_unused]] att_title, const std::string &[[maybe_unused]] title) {}
	virtual void add_navigation() {}
	virtual void add_summary_list(const std::vector<std::string> &[[maybe_unused]] list) {}
	virtual void add_table(const std::vector<std::string> &[[maybe_unused]] system_data,
			struct table_attributes *[[maybe_unused]] tb_attr)     {}
};

