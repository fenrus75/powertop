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
	add_exact("<img alt=\"PowerTop\" class=\"pwtop_logo\" src=\"data:image/png;base64,"
		"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"
		"APNNNssrHc8WAAAAAElFTkSuQmCC\">");
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

