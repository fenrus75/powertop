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
 * Generic report generator.
 * Written by Igor Zhbanov <i.zhbanov@samsung.com>
 * 2012.10 */

#ifndef _REPORT_MAKER_H_
#define _REPORT_MAKER_H_

/* This report generator implements the following document structure:
 *	body
 *	  \---> section
 *		  |---> header
 *		  |---> paragraph
 *		  \---> table
 *			  \---> table row
 *				  \---> table cell
 *
 * The report body consists of a number of sections (a.k.a. <div>s,
 * a.k.a. tabs).
 * Each section can contain headers (<h1>, <h2>, <h3>), paragraphs (<p>)
 * and tables (<table>).
 *
 * A header is a single line of text.
 *
 * Paragraphs can contain only text.
 *
 * A table consists of table rows. A table row consists of table cells.
 * A table cell can contain only text.
 *
 * Each section, table, row or cell could have its own formatting.
 * To distinguish elements from others of its type, each element could have
 * an unique identifier (see enums section_type, table_type, row_type and
 * cell_type below). These identifiers are used in formatter implementations
 * to produce special formatting.
 *
 * Example of usage:
 *	report_maker report(REPORT_OFF);
 *
 *	report.set_type(REPORT_HTML);
 *	report.begin_section();
 *		report.add_header("Big report");
 *		report.begin_paragraph("Some text");
 *		report.begin_table();
 *			report.begin_row();
 *				report.begin_cell();
 *					report.add("Something");
 *				report.begin_cell(CELL_SPECIAL);
 *					report.add("Foo bar");
 *	report.finish_report();
 *	const char *result = report.get_result();
 */

#include <stdarg.h>

#include <string>

/* Conditional gettext. We need original strings for CSV. */
#define __(STRING) \
	((report.get_type() == REPORT_CSV) ? (STRING) : gettext(STRING))

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif /* UNUSED */

/* ************************************************************************ */

enum report_type {
	REPORT_OFF,
	REPORT_HTML,
	REPORT_CSV
};

/* ************************************************************************ */

enum section_type {
	SECTION_DEFAULT,
	SECTION_SYSINFO,
	SECTION_CPUIDLE,
	SECTION_CPUFREQ,
	SECTION_DEVPOWER,
	SECTION_SOFTWARE,
	SECTION_SUMMARY,
	SECTION_TUNING,
	SECTION_MAX /* Must be last in this enum */
};

/* ************************************************************************ */

enum table_type {
	TABLE_DEFAULT,
	TABLE_WIDE,
	TABLE_MAX /* Must be last in this enum */
};

/* ************************************************************************ */

enum row_type {
	ROW_DEFAULT,
	ROW_SYSINFO,
	ROW_DEVPOWER,
	ROW_SOFTWARE,
	ROW_SUMMARY,
	ROW_TUNABLE,
	ROW_TUNABLE_BAD,
	ROW_MAX /* Must be last in this enum */
};

/* ************************************************************************ */

enum cell_type {
	CELL_DEFAULT,
	CELL_SYSINFO,
	CELL_FIRST_PACKAGE_HEADER,
	CELL_EMPTY_PACKAGE_HEADER,
	CELL_CORE_HEADER,
	CELL_CPU_CSTATE_HEADER,
	CELL_CPU_PSTATE_HEADER,
	CELL_STATE_NAME,
	CELL_EMPTY_PACKAGE_STATE,
	CELL_PACKAGE_STATE_VALUE,
	CELL_CORE_STATE_VALUE,
	CELL_CPU_STATE_VALUE,
	CELL_SEPARATOR,
	CELL_DEVPOWER_HEADER,
	CELL_DEVPOWER_DEV_NAME,
	CELL_DEVPOWER_POWER,
	CELL_DEVPOWER_UTIL,
	CELL_DEVACTIVITY_PROCESS,
	CELL_DEVACTIVITY_DEVICE,
	CELL_SOFTWARE_HEADER,
	CELL_SOFTWARE_PROCESS,
	CELL_SOFTWARE_DESCRIPTION,
	CELL_SOFTWARE_POWER,
	CELL_SUMMARY_HEADER,
	CELL_SUMMARY_CATEGORY,
	CELL_SUMMARY_DESCRIPTION,
	CELL_SUMMARY_ITEM,
	CELL_TUNABLE_HEADER,
	CELL_UNTUNABLE_HEADER,
	CELL_MAX /* Must be last in this enum */
};

/* ************************************************************************ */

class report_formatter;

class report_maker
{
public:
	report_maker(report_type t);
       ~report_maker();

	report_type get_type();
	void set_type(report_type t);

	void addf(const char *fmt, ...)
				__attribute__ ((format (printf, 2, 3)));

	void finish_report();
	const char *get_result();
	void clear_result();

	void add(const char *str);

	void add_header(const char *header, int level = 2);
	void begin_section(section_type stype = SECTION_DEFAULT);
	void begin_table(table_type ttype = TABLE_DEFAULT);
	void begin_row(row_type rtype = ROW_DEFAULT);
	void begin_cell(cell_type ctype = CELL_DEFAULT);
	void add_empty_cell();
	void begin_paragraph();

	void set_cpu_number(int nr);

private:
	void setup_report_formatter();

	void end_section();
	void end_table();
	void end_row();
	void end_cell();
	void end_paragraph();

	report_type type;
	report_formatter *formatter;
	bool cell_opened, row_opened, table_opened, section_opened,
	     paragraph_opened;
};

#endif /* _REPORT_MAKER_H_ */
