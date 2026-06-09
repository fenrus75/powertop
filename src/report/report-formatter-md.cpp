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

#include <cstdio>
#include <format>
#include "report-formatter-md.h"
#include "report-data-html.h"
#include "lib.h"

report_formatter_md::report_formatter_md()
{
}

void report_formatter_md::finish_report()
{
}

void report_formatter_md::add_logo()
{
	add_exact(std::format("# {}\n\n", _("PowerTOP Report")));
}

void report_formatter_md::add_header()
{
	add_exact("______________________________________________________________________\n\n");
}

void report_formatter_md::add_div([[maybe_unused]] const struct tag_attr *div_attr)
{
}

void report_formatter_md::add_title([[maybe_unused]] const struct tag_attr *title_attr, const std::string &title)
{
	add_exact(std::format("## {}\n\n", title));
}

void report_formatter_md::add_summary_list(const std::vector<std::string> &list)
{
	for (size_t i = 0; i + 1 < list.size(); i += 2) {
		add_exact(std::format("- **{}**: {}\n", list[i], list[i + 1]));
	}
	add_exact("\n");
}

void report_formatter_md::add_table(const std::vector<std::string> &system_data, const struct table_attributes *tb_attr)
{
	if (tb_attr->rows <= 0 || tb_attr->cols <= 0) {
		return;
	}

	if (tb_attr->pos_table_title == L && tb_attr->cols == 2) {
		add_exact(std::format("| {} | {} |\n", _("Property"), _("Value")));
		add_exact("|---|---|\n");
	}

	for (int i = 0; i < tb_attr->rows; i++) {
		add_exact("| ");
		for (int j = 0; j < tb_attr->cols; j++) {
			int offset = i * tb_attr->cols + j;
			if (offset < (int)system_data.size()) {
				std::string val = system_data[offset];
				if (val == "&nbsp;") {
					val = " ";
				}
				add_exact(escape_string(val));
			}
			if (j < tb_attr->cols - 1) {
				add_exact(" | ");
			} else {
				add_exact(" |");
			}
		}
		add_exact("\n");
		if (i == 0 && tb_attr->pos_table_title != L) {
			add_exact("|");
			for (int j = 0; j < tb_attr->cols; j++) {
				add_exact("---|");
			}
			add_exact("\n");
		}
	}
	add_exact("\n");
}

std::string report_formatter_md::get_result() const
{
	std::string res = report_formatter_string_base::get_result();
	while (!res.empty() && isspace(res.back())) {
		res.pop_back();
	}
	if (!res.empty()) {
		res += "\n";
	}
	return res;
}

std::string report_formatter_md::escape_string(const std::string &str)
{
	std::string res;
	for (char c : str) {
		if (c == '|' || c == '\\' || c == '`' || c == '*') {
			res += '\\';
		}
		res += c;
	}
	return res;
}
