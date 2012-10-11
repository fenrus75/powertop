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

#define _BSD_SOURCE

/* Uncomment to disable asserts */
/*#define NDEBUG*/

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "report-formatter-html.h"
#include "css.h" /* For HTML-report header */

/* ************************************************************************ */

#ifdef EXTERNAL_CSS_FILE /* Where is it defined? */
static const char report_html_alternative_head[] =
	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
		"\"http://www.w3.org/TR/html4/loose.dtd\">\n"
	"<html>\n"
	"<head>\n"
	"<title>PowerTOP report</title>\n"
	"<link rel=\"stylesheet\" href=\"powertop.css\">\n"
	"</head>\n"
	"<body>\n";
#endif /* EXTERNAL_CSS_FILE */

/* ************************************************************************ */

static const char report_html_footer[] =
	"</body>\n"
	"</html>\n";

/* ************************************************************************ */

void
report_formatter_html::init_markup()
{
	/*	    stype		id */
	set_section(SECTION_DEFAULT);
	set_section(SECTION_SYSINFO,	"system");
	set_section(SECTION_CPUIDLE,	"cpuidle");
	set_section(SECTION_CPUFREQ,	"cpufreq");
	set_section(SECTION_DEVPOWER,	"device");
	set_section(SECTION_SOFTWARE,	"software");
	set_section(SECTION_SUMMARY,	"summary");
	set_section(SECTION_TUNING,	"tuning");

	/*	  ttype		 width */
	set_table(TABLE_DEFAULT, "");
	set_table(TABLE_WIDE,    "100%");

	/*	 ctype				is_header, width, colspan */
	set_cell(CELL_DEFAULT);
	set_cell(CELL_SEPARATOR,		false,	"2%");
	set_cell(CELL_SYSINFO,			false,	"20%");
	set_cell(CELL_FIRST_PACKAGE_HEADER,	true,	"25%",	2);
	set_cell(CELL_EMPTY_PACKAGE_HEADER,	true,	"",	2);
	set_cell(CELL_CORE_HEADER,		true,	"25%",	2);
	set_cell(CELL_CPU_CSTATE_HEADER,	true,	"",	3);
	set_cell(CELL_CPU_PSTATE_HEADER,	true,	"",	2);
	set_cell(CELL_STATE_NAME,		false,	"10%");
	set_cell(CELL_EMPTY_PACKAGE_STATE,	false,	"",	2);
	set_cell(CELL_PACKAGE_STATE_VALUE);
	set_cell(CELL_CORE_STATE_VALUE);
	set_cell(CELL_CPU_STATE_VALUE);
	set_cell(CELL_DEVPOWER_HEADER,		true,	"10%");
	set_cell(CELL_DEVPOWER_DEV_NAME,	true);
	set_cell(CELL_DEVPOWER_POWER);
	set_cell(CELL_DEVPOWER_UTIL);
	set_cell(CELL_DEVACTIVITY_PROCESS,	true,	"40%");
	set_cell(CELL_DEVACTIVITY_DEVICE,	true);
	set_cell(CELL_SOFTWARE_HEADER,		true,	"10%");
	set_cell(CELL_SOFTWARE_PROCESS,		true,	"10%");
	set_cell(CELL_SOFTWARE_DESCRIPTION,	true);
	set_cell(CELL_SOFTWARE_POWER);
	set_cell(CELL_SUMMARY_HEADER,		true,	"10%");
	set_cell(CELL_SUMMARY_CATEGORY,		true,	"10%");
	set_cell(CELL_SUMMARY_DESCRIPTION,	true);
	set_cell(CELL_SUMMARY_ITEM);
	set_cell(CELL_TUNABLE_HEADER,		true);
}

/* ************************************************************************ */

const char *
report_formatter_html::get_row_style(row_type rtype)
{
	switch (rtype) {
		case ROW_SYSINFO:
			return get_style_pair("system_even", "system_odd");
		case ROW_DEVPOWER:
			return get_style_pair("device_even", "device_odd");
		case ROW_SOFTWARE:
		case ROW_SUMMARY:
			return get_style_pair("process_even", "process_odd");
		case ROW_TUNABLE:
			return get_style_pair("tunable_even", "tunable_odd");
		case ROW_TUNABLE_BAD:
			return get_style_pair("tunable_even_bad",
							   "tunable_odd_bad");
		default:
			return "";
	};
}

/* ************************************************************************ */

const char *
report_formatter_html::get_cell_style(cell_type ctype)
{
	switch (ctype) {
		case CELL_FIRST_PACKAGE_HEADER:
		case CELL_EMPTY_PACKAGE_HEADER:
			return "package_header";
		case CELL_CORE_HEADER:
			return "core_header";
		case CELL_CPU_CSTATE_HEADER:
		case CELL_CPU_PSTATE_HEADER:
			return "cpu_header";
		case CELL_STATE_NAME:
			return get_style_pair("cpu_even_freq",
					      "cpu_odd_freq");
		case CELL_PACKAGE_STATE_VALUE:
			return get_style_pair("package_even",
					      "package_odd");
		case CELL_CORE_STATE_VALUE:
			return get_style_pair("core_even",
					      "core_odd");
		case CELL_CPU_STATE_VALUE:
			return get_style_quad("cpu_even_even", "cpu_even_odd",
					      "cpu_odd_even", "cpu_odd_odd",
					      cpu_nr);
		case CELL_DEVPOWER_DEV_NAME:
		case CELL_DEVACTIVITY_PROCESS:
		case CELL_DEVACTIVITY_DEVICE:
			return "device";
		case CELL_DEVPOWER_POWER:
			return "device_power";
		case CELL_DEVPOWER_UTIL:
			return "device_util";
		case CELL_SOFTWARE_PROCESS:
		case CELL_SOFTWARE_DESCRIPTION:
		case CELL_SUMMARY_CATEGORY:
		case CELL_SUMMARY_DESCRIPTION:
			return "process";
		case CELL_SOFTWARE_POWER:
		case CELL_SUMMARY_ITEM:
			return "process_power";
		case CELL_TUNABLE_HEADER:
			return "tunable";
		default:
			return "";
	};
}

/* ************************************************************************ */

report_formatter_html::report_formatter_html()
{
	init_markup();
	add_doc_header();
}

/* ************************************************************************ */

const char *
report_formatter_html::get_style_pair(const char *even, const char *odd)
{
	if (!(table_row_number & 1) && even)
		return even;

	if ((table_row_number & 1) && odd)
		return odd;

	return "";
}

/* ************************************************************************ */

const char *
report_formatter_html::get_style_quad(const char *even_even,
				   const char *even_odd, const char *odd_even,
				   const char *odd_odd, int alt_cell_number)
{
	int cell;

	cell = (alt_cell_number != -1 ? alt_cell_number : table_cell_number);
	if (!(table_row_number & 1) && !(cell & 1) && even_even)
		return even_even;

	if (!(table_row_number & 1) && (cell & 1) && even_odd)
		return even_odd;

	if ((table_row_number & 1) && !(cell & 1) && odd_even)
		return odd_even;

	if ((table_row_number & 1) && (cell & 1) && odd_odd)
		return odd_odd;

	return "";
}

/* ************************************************************************ */

void
report_formatter_html::finish_report()
{
	add_doc_footer();
}

/* ************************************************************************ */

void
report_formatter_html::set_section(section_type stype, const char *id)
{
	sections[stype].id = id;
}

/* ************************************************************************ */

void
report_formatter_html::set_table(table_type ttype, const char *width)
{
	tables[ttype].width = width;
}

/* ************************************************************************ */

void
report_formatter_html::set_cell(cell_type ctype, bool is_header,
			     const char *width, int colspan)
{
	cells[ctype].is_header = is_header;
	cells[ctype].width     = width;
	cells[ctype].colspan   = colspan;
}

/* ************************************************************************ */

void
report_formatter_html::add_doc_header()
{
#ifdef EXTERNAL_CSS_FILE /* Where is it defined? */
	add_exact(report_html_alternative_head);
#else /* !EXTERNAL_CSS_FILE */
	add_exact(css);
#endif /* !EXTERNAL_CSS_FILE */
}

/* ************************************************************************ */

void
report_formatter_html::add_doc_footer()
{
	add_exact(report_html_footer);
}

/* ************************************************************************ */

void
report_formatter_html::add_header(const char *header, int level)
{
	addf_exact("<h%d>", level);
	add(header);
	addf_exact("</h%d>\n", level);
}

/* ************************************************************************ */

void
report_formatter_html::begin_section(section_type stype)
{
	if (sections[stype].id[0])
		addf_exact("<div id=\"%s\">", sections[stype].id);
	else
		add_exact("<div>");
}

/* ************************************************************************ */

void
report_formatter_html::end_section()
{
	add_exact("</div>\n\n");
}

/* ************************************************************************ */

void
report_formatter_html::begin_table(table_type ttype)
{
	table_row_number = 0;
	add_exact("<table");
	if (tables[ttype].width[0])
		addf_exact(" width=\"%s\"", tables[ttype].width);

	add_exact(">\n");
}

/* ************************************************************************ */

void
report_formatter_html::end_table()
{
	add_exact("</table>\n\n");
}

/* ************************************************************************ */

void
report_formatter_html::begin_row(row_type rtype)
{
	const char *row_style;

	table_cell_number = 0;
	add_exact("<tr");
	row_style = get_row_style(rtype);
	if (row_style[0])
		addf_exact(" class=\"%s\"", row_style);

	add_exact(">\n");
}

/* ************************************************************************ */

void
report_formatter_html::end_row()
{
	add_exact("</tr>\n");
	table_row_number++;
}

/* ************************************************************************ */

void
report_formatter_html::begin_cell(cell_type ctype)
{
	const char *cell_style;

	current_cell = ctype;
	if (cells[ctype].is_header)
		add_exact("\t<th");
	else
		add_exact("\t<td");

	if (cells[ctype].width[0])
		addf_exact(" width=\"%s\"", cells[ctype].width);

	if (cells[ctype].colspan > 1)
		addf_exact(" colspan=\"%d\"", cells[ctype].colspan);

	cell_style = get_cell_style(ctype);
	if (cell_style[0])
		addf_exact(" class=\"%s\"", cell_style);

	add_exact(">");
}

/* ************************************************************************ */

void
report_formatter_html::end_cell()
{
	if (!cells[current_cell].is_header)
		add_exact("</td>\n");
	else
		add_exact("</th>\n");

	table_cell_number++;
}

/* ************************************************************************ */

void
report_formatter_html::add_empty_cell()
{
	add_exact("&nbsp;");
}

/* ************************************************************************ */

void
report_formatter_html::begin_paragraph()
{
	add_exact("<p>");
}

/* ************************************************************************ */

void
report_formatter_html::end_paragraph()
{
	add_exact("</p>\n");
}

/* ************************************************************************ */

std::string
report_formatter_html::escape_string(const char *str)
{
	std::string res;

	assert(str);

	for (const char *i = str; *i; i++) {
		switch (*i) {
			case '<':
				res += "&lt;";
				continue;
			case '>':
				res += "&gt;";
				continue;
			case '&':
				res += "&amp;";
				continue;
#ifdef REPORT_HTML_ESCAPE_QUOTES
			case '"':
				res += "&quot;";
				continue;
			case '\'':
				res += "&apos;";
				continue;
#endif /* REPORT_HTML_ESCAPE_QUOTES */
		}

		res += *i;
	}

	return res;
}

/* ************************************************************************ */

void
report_formatter_html::set_cpu_number(int nr)
{
	assert(nr >= 0);

	cpu_nr = nr;
}
