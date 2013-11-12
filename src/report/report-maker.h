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
 *		  |---> Title
 *		  |---> paragraph
 *		  |---> table
 *		  |---> list
 *
 * The report body consists of a number of sections (a.k.a. <div>s,
 * a.k.a. tabs).
 * Each section can contain titles (<h2>), paragraphs (<p>)
 * and tables (<table>).
 *
 * A header is a single line of text.
 *
 * Paragraphs can contain only text.
 *
 *
 * Each section, table, row or cell could have its own formatting.
 *
 * Example of usage:
 *	report_maker report(REPORT_OFF);
 *
 *	report.set_type(REPORT_HTML);
 *	report.add_div
 *		report.add_title
 *		report.add_list
 *		report.add_table
 *	report.finish_report();
 *	const char *result = report.get_result();
 */

#include <stdarg.h>

#include <string>
using namespace std;
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

	/* *** Report Style *** */
	void add_header();
	void end_header();
	void add_logo();
	void add_div(struct tag_attr *div_attr);
	void end_div();
	void add_title(struct tag_attr *att_title, const char *title);
	void add_navigation();
	void add_summary_list(string *list, int size);
	void add_table(string *system_data, struct table_attributes *tb_attr);

private:
	void setup_report_formatter();
	report_type type;
	report_formatter *formatter;
};
#endif /* _REPORT_MAKER_H_ */
