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
	table_opened	 = false;
	cell_opened	 = false;
	row_opened	 = false;
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

	if (table_opened)
		end_table();
	else if (paragraph_opened)
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

	if (table_opened)
		end_table();
	else if (paragraph_opened)
		end_paragraph();

	section_opened = false;
	formatter->end_section();
}

/* ************************************************************************ */

void
report_maker::begin_table(table_type ttype)
{
	assert(ttype >= 0 && ttype < TABLE_MAX);
	assert(section_opened);

	if (table_opened)
		end_table(); /* Close previous */
	else if (paragraph_opened)
		end_paragraph();

	table_opened = true;
	formatter->begin_table(ttype);
}

/* ************************************************************************ */

void
report_maker::end_table()
{
	assert(section_opened);
	assert(table_opened);

	if (row_opened)
		end_row();

	table_opened = false;
	formatter->end_table();
}

/* ************************************************************************ */

void
report_maker::begin_row(row_type rtype)
{
	assert(section_opened);
	assert(table_opened);
	assert(rtype >= 0 && rtype < ROW_MAX);

	if (row_opened)
		end_row(); /* Close previous */

	row_opened = true;
	formatter->begin_row(rtype);
}

/* ************************************************************************ */

void
report_maker::end_row()
{
	assert(section_opened);
	assert(table_opened);
	assert(row_opened);

	if (cell_opened)
		end_cell();

	row_opened = false;
	formatter->end_row();
}

/* ************************************************************************ */

void
report_maker::begin_cell(cell_type ctype)
{
	assert(section_opened);
	assert(table_opened);
	assert(row_opened);
	assert(ctype >= 0 && ctype < CELL_MAX);

	if (cell_opened)
		end_cell(); /* Close previous */

	cell_opened = true;
	formatter->begin_cell(ctype);
}

/* ************************************************************************ */

void
report_maker::end_cell()
{
	assert(section_opened);
	assert(table_opened);
	assert(row_opened);
	assert(cell_opened);

	cell_opened = false;
	formatter->end_cell();
}

/* ************************************************************************ */

void
report_maker::add_empty_cell()
{
	formatter->add_empty_cell();
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

	if (table_opened)
		end_table();
	else if (paragraph_opened)
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
