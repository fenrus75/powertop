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
 * HTML report generator.
 * Written by Igor Zhbanov <i.zhbanov@samsung.com>
 * 2012.10 */

#ifndef _REPORT_FORMATTER_HTML_H_
#define _REPORT_FORMATTER_HTML_H_

#include <string>

#include "report-formatter-base.h"

/* Whether to replace " and ' in HTML by &quot; and &apos; respectively */
/*#define REPORT_HTML_ESCAPE_QUOTES*/

/* ************************************************************************ */

struct html_section {
	const char *id;
};

/* ************************************************************************ */

struct html_table {
	const char *width;
};

/* ************************************************************************ */

struct html_cell {
	bool is_header;
	const char *width;
	int colspan;
};

/* ************************************************************************ */

class report_formatter_html: public report_formatter_string_base
{
public:
	report_formatter_html();

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
	/* Document structure related functions */
	void init_markup();
	void set_section(section_type stype, const char *id = "");
	void set_table(table_type ttype, const char *width = "");
	void set_cell(cell_type ctype, bool is_header = false,
			const char *width = "", int colspan = 1);

	/* HTML table elements CSS classes */
	const char *get_cell_style(cell_type ctype);
	const char *get_row_style(row_type rtype);
	const char *get_style_pair(const char *even, const char *odd);
	const char *get_style_quad(const char *even_even,
				   const char *even_odd,
				   const char *odd_even,
				   const char *odd_odd,
				   int alt_cell_number = -1);

	void add_doc_header();
	void add_doc_footer();

	std::string escape_string(const char *str);

	html_section sections[SECTION_MAX];
	html_table tables[TABLE_MAX];
	html_cell cells[CELL_MAX];
	size_t table_row_number, table_cell_number;
	cell_type current_cell;
	int cpu_nr;
};

#endif /* _REPORT_FORMATTER_HTML_H_ */
