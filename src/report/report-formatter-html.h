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
#include "report-data-html.h"

using namespace std;

/* Whether to replace " and ' in HTML by &quot; and &apos; respectively */
/*#define REPORT_HTML_ESCAPE_QUOTES*/

/* ************************************************************************ */

struct html_section {
	const char *id;
};

/* ************************************************************************ */

class report_formatter_html: public report_formatter_string_base
{
public:
	report_formatter_html();
	void finish_report();

	/* Report Style */
	void add_logo();
	void add_header();
	void end_header();
	void add_div(struct tag_attr *div_attr);
	void end_div();
	void add_title(struct tag_attr *title_att, const char *title);
	void add_navigation();
	void add_summary_list(string *list, int size);
	void add_table(string *system_data, struct table_attributes *tb_attr);

private:
	/* Document structure related functions */
	void init_markup();
	void add_doc_header();
	void add_doc_footer();
	string escape_string(const char *str);

};

#endif /* _REPORT_FORMATTER_HTML_H_ */
