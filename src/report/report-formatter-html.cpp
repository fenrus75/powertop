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
	"</div>\n"
	"</body>\n"
	"</html>\n";

/* ************************************************************************ */

void
report_formatter_html::init_markup()
{
	/*here all html code*/
}

/* ************************************************************************ */

report_formatter_html::report_formatter_html()
{
	init_markup();
	add_doc_header();
}

/* ************************************************************************ */

void
report_formatter_html::finish_report()
{
	add_doc_footer();
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
string
report_formatter_html::escape_string(const char *str)
{
	string res;

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


/* Report Style */
void
report_formatter_html::add_logo()
{
	add_exact("<img alt=\"PowerTop\" class=\"pwtop_logo\" src=\"./PowerTop.png\">");
}

void
report_formatter_html::add_header()
{
	add_exact("<header id=\"main_header\">\n");
}

void
report_formatter_html::end_header()
{
	add_exact("</header>\n\n");
}

void
report_formatter_html::add_div(struct tag_attr * div_attr)
{
	string empty="";
	string tmp_str;

	if (div_attr->css_class == empty && div_attr->css_id == empty)
		add_exact("<div>\n");

	else if (div_attr->css_class == empty && div_attr->css_id != empty)
		addf_exact("<div id=\"%s\">\n", div_attr->css_id);

	else if (div_attr->css_class != empty && div_attr->css_id == empty)
		addf_exact("<div class=\"%s\">\n", div_attr->css_class);

	else if (div_attr->css_class != empty && div_attr->css_id != empty)
		addf_exact("<div class=\"%s\" id=\"%s\">\n",
				div_attr->css_class, div_attr->css_id);
}

void
report_formatter_html::end_div()
{
	add_exact("</div>\n");
}

void
report_formatter_html::add_title(struct tag_attr *title_att, const char *title)
{
	addf_exact("<h2 class=\"%s\"> %s </h2>\n", title_att->css_class, title);
}

void
report_formatter_html::add_navigation()
{
	add_exact("<br/><div id=\"main_menu\"> </div>\n");
}

void
report_formatter_html::add_summary_list(string *list, int size)
{
	int i;
	add_exact("<div><br/> <ul>\n");
	for (i=0; i < size; i+=2){
		addf_exact("<li class=\"summary_list\"> <b> %s </b> %s </li>",
				list[i].c_str(), list[i+1].c_str());
	}
	add_exact("</ul> </div> <br />\n");
}


void
report_formatter_html::add_table(string *system_data, struct table_attributes* tb_attr)
{
	int i, j;
	int offset=0;
	string  empty="";

	if (tb_attr->table_class == empty)
		add_exact("<table>\n");
	else
		addf_exact("<table class=\"%s\">\n", tb_attr->table_class);

	for (i=0; i < tb_attr->rows; i++){
		if (tb_attr->tr_class == empty)
			add_exact("<tr> ");
		else
			addf_exact("<tr class=\"%s\"> ", tb_attr->tr_class);

		for (j=0; j < tb_attr->cols; j++){
			offset = i * (tb_attr->cols) + j;

			if (tb_attr->pos_table_title == T &&  i==0)
				addf_exact("<th class=\"%s\"> %s </th> ",
				tb_attr->th_class,system_data[offset].c_str());
			else if (tb_attr->pos_table_title == L &&  j==0)
				addf_exact("<th class=\"%s\"> %s </th> ",
				tb_attr->th_class, system_data[offset].c_str());
			else if (tb_attr->pos_table_title == TL && ( i==0 || j==0 ))
				addf_exact("<th class=\"%s\"> %s </th> ",
				tb_attr->th_class, system_data[offset].c_str());
			else if (tb_attr->pos_table_title == TC && ((i % tb_attr->title_mod ) == 0))
				addf_exact("<th class=\"%s\"> %s </th> ", tb_attr->th_class,
					system_data[offset].c_str());
			else if (tb_attr->pos_table_title == TLC && ((i % tb_attr->title_mod) == 0 || j==0))
				addf_exact("<th class=\"%s\"> %s </th> ", tb_attr->th_class,
				system_data[offset].c_str());
			else
				if ( tb_attr->td_class == empty)
					addf_exact("<td > %s </td> ", system_data[offset].c_str());
				else
					addf_exact("<td class=\"%s\"> %s </td> ", tb_attr->td_class,
						system_data[offset].c_str());
		}
		add_exact("</tr>\n");
	}
	add_exact("</table>\n");
}

