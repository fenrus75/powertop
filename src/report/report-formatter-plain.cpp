/* Copyright (c) 2016 Jaroslav Škarvada <jskarvad@redhat.com>
 * Based on CSV formatter code by Igor Zhbanov <i.zhbanov@samsung.com>
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
 */

/* Uncomment to disable asserts */
/*#define NDEBUG*/

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "report-formatter-plain.h"
#include "report-data-html.h"


/* ************************************************************************ */
report_formatter_plain::report_formatter_plain()
{
	/* Do nothing special  */
}

/* ************************************************************************ */
void
report_formatter_plain::finish_report()
{
	/* Do nothing special */
}

string
report_formatter_plain::escape_string(const char *str)
{
	return string(str);
}


/* Report Style */
void
report_formatter_plain::add_header()
{
	add_exact("\n");
}

void
report_formatter_plain::end_header()
{
	/* Do nothing */
}

void
report_formatter_plain::add_logo()
{
	add_exact("= PowerTOP =\n");
}


void
report_formatter_plain::add_div(struct tag_attr * div_attr)
{
	add_exact("\n");
}

void
report_formatter_plain::end_div()
{
	/*Do nothing*/
}

void
report_formatter_plain::add_title(struct tag_attr *title_att, const char *title)
{
	add_exact("\n");
	addf_exact("- %s -\n", title);
}

void
report_formatter_plain::add_navigation()
{
	/* No nav in plain - thinking on table of contents */
}

void
report_formatter_plain::add_summary_list(string *list, int size)
{
	int i;
	add_exact("\n");
	for (i=0; i < size; i += 2){
		addf_exact("%s %s", list[i].c_str(), list[i + 1].c_str());
		if(i < (size - 1))
			add_exact("\n");
	}
	add_exact("\n");
}

void
report_formatter_plain::add_table(string *system_data, struct table_attributes* tb_attr)
{
	int i, j;
	int offset=0;
	string tmp_str="";
	int empty_row=0;
	add_exact("\n");
	for (i=0; i < tb_attr->rows; i++){
		for (j=0; j < tb_attr->cols; j++){
			offset = i * (tb_attr->cols) + j;
			tmp_str=system_data[offset];

			if(tmp_str == "&nbsp;")
				empty_row+=1;
			else{
				addf_exact("%s", system_data[offset].c_str());
				if(j < (tb_attr->cols - 1))
					add_exact(" ");
			}
		}
		if(empty_row < tb_attr->cols)
		add_exact("\n");
		empty_row=0;
	}
}
