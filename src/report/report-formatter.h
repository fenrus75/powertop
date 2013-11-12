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
 * report_formatter interface.
 * Written by Igor Zhbanov <i.zhbanov@samsung.com>
 * 2012.10 */

#ifndef _REPORT_FORMATTER_H_
#define _REPORT_FORMATTER_H_

#include "report-maker.h"
using namespace std;

class report_formatter
{
public:
	virtual ~report_formatter() {}

	virtual void finish_report() {}
	virtual const char *get_result() {return "Basic report_formatter::get_result() call\n";}
	virtual void clear_result() {}

	virtual void add(const char *str) {}
	virtual void addv(const char *fmt, va_list ap) {}

	/* *** Report Style *** */
	virtual void add_logo() {}
	virtual void add_header() {}
	virtual void end_header() {}
	virtual void add_div(struct tag_attr *div_attr) {}
	virtual void end_div() {}
	virtual void add_title(struct tag_attr *att_title, const char *title) {}
	virtual void add_navigation() {}
	virtual void add_summary_list(string *list, int size) {}
	virtual void add_table(string *system_data,
			struct table_attributes *tb_attr)     {}
};

#endif /* _REPORT_FORMATTER_H_ */
