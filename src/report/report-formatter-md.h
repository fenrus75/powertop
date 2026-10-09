/*
 * Copyright (C) 2026 Intel Corporation
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once
#include "report-formatter-base.h"

class report_formatter_md : public report_formatter_string_base
{
public:
	report_formatter_md();
	void finish_report() override;
	void add_logo() override;
	void add_header() override;
	void add_div(const struct tag_attr *div_attr) override;
	void add_title(const struct tag_attr *title_attr, const std::string &title) override;
	void add_summary_list(const std::vector<std::string> &list) override;
	void add_table(const std::vector<std::string> &system_data, const struct table_attributes *tb_attr) override;

	std::string get_result() const override;

protected:
	std::string escape_string(const std::string &str) override;
};
