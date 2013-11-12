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

/* Uncomment to disable asserts */
/*#define NDEBUG*/

#include <assert.h>
#include <stdarg.h>

#include "report-maker.h"
#include "report-formatter-csv.h"
#include "report-formatter-html.h"

/* ************************************************************************ */

report_maker::report_maker(report_type t)
{
	type = t;
	formatter = NULL;
	setup_report_formatter();
	clear_result(); /* To reset state and add document header */
}

/* ************************************************************************ */

report_maker::~report_maker()
{
	if (formatter)
		delete formatter;
}

/* ************************************************************************ */

void
report_maker::finish_report()
{
	if (section_opened)
		end_section();

	formatter->finish_report();
}

/* ************************************************************************ */

const char *
report_maker::get_result()
{
	return formatter->get_result();
}

/* ************************************************************************ */

void
report_maker::clear_result()
{
	formatter->clear_result();
	section_opened	 = false;
	paragraph_opened = false;
}

/* ************************************************************************ */

report_type
report_maker::get_type()
{
	return type;
}

/* ************************************************************************ */

void
report_maker::set_type(report_type t)
{
	clear_result();
	type = t;
	setup_report_formatter();
}

/* ************************************************************************ */

void
report_maker::setup_report_formatter()
{
	if (formatter)
		delete formatter;

	if (type == REPORT_HTML)
		formatter = new report_formatter_html();
	else if (type == REPORT_CSV)
		formatter = new report_formatter_csv();
	else if (type == REPORT_OFF)
		formatter = new report_formatter();
	else
		assert(false);
}

/* ************************************************************************ */

void
report_maker::add(const char *str)
{
	assert(str);
	assert(section_opened);

	formatter->add(str);
}

/* ************************************************************************ */

void
report_maker::addf(const char *fmt, ...)
{
	va_list ap;

	assert(fmt);
	assert(section_opened);

	va_start(ap, fmt);
	formatter->addv(fmt, ap);
	va_end(ap);
}

/* ************************************************************************ */

void
report_maker::add_header(const char *header, int level)
{
	assert(header);
	assert(section_opened);
	assert(level > 0 && level < 4);

	if (paragraph_opened)
		end_paragraph();

	formatter->add_header(header, level);
}

/* ************************************************************************ */

void
report_maker::begin_section(section_type stype)
{
	assert(stype >= 0 && stype < SECTION_MAX);

	if (section_opened)
		end_section(); /* Close previous */

	section_opened = true;
	formatter->begin_section(stype);
}

/* ************************************************************************ */

void
report_maker::end_section()
{
	assert(section_opened);

	if (paragraph_opened)
		end_paragraph();

	section_opened = false;
	formatter->end_section();
}

/* ************************************************************************ */

void
report_maker::set_cpu_number(int nr)
{
	formatter->set_cpu_number(nr);
}

/* ************************************************************************ */

void
report_maker::begin_paragraph()
{
	assert(section_opened);

	if (paragraph_opened)
		end_paragraph(); /* Close previous */

	paragraph_opened = true;
	formatter->begin_paragraph();
}

/* ************************************************************************ */

void
report_maker::end_paragraph()
{
	assert(paragraph_opened);

	paragraph_opened = false;
	formatter->end_paragraph();
}


/* *** Report Style *** */
void
report_maker::add_logo()
{
	formatter->add_logo();
}

void
report_maker::add_header()
{
	formatter->add_header();
}

void
report_maker::end_hheader()
{
	formatter->end_hheader();
}

void
report_maker::add_title(struct tag_attr *att_title, const char *title)
{
	formatter->add_title(att_title, title);
}

void
report_maker::add_div(struct tag_attr * div_attr)
{
	formatter->add_div(div_attr);
}

void
report_maker::end_div()
{
	formatter->end_div();
}

void
report_maker::add_navigation()
{
	formatter->add_navigation();
}

void
report_maker::add_summary_list(string *list, int size)
{
	formatter->add_summary_list(list, size);
}

void
report_maker::add_table(string *system_data, struct table_attributes *tb_attr)
{
	formatter->add_table(system_data, tb_attr);
}

