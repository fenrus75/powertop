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
 * Null report generator.
 * Written by Igor Zhbanov <i.zhbanov@samsung.com>
 * 2012.10 */

#include <stdarg.h>

#include "report-formatter-null.h"

/* ************************************************************************ */

report_formatter_null::report_formatter_null()
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::finish_report()
{
	/* Do nothing */
}

/* ************************************************************************ */

const char *
report_formatter_null::get_result()
{
	return "";
}

/* ************************************************************************ */

void
report_formatter_null::clear_result()
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::add(const char *str UNUSED)
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::addv(const char *fmt UNUSED, va_list ap UNUSED)
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::add_header(const char *header UNUSED, int level UNUSED)
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::begin_section(section_type stype UNUSED)
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::end_section()
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::begin_table(table_type ttype UNUSED)
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::end_table()
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::begin_row(row_type rtype UNUSED)
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::end_row()
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::begin_cell(cell_type ctype UNUSED)
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::end_cell()
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::add_empty_cell()
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::begin_paragraph()
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::end_paragraph()
{
	/* Do nothing */
}

/* ************************************************************************ */

void
report_formatter_null::set_cpu_number(int nr UNUSED)
{
	/* Do nothing */
}
