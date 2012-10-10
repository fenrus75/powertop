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
 * Common part of report_formatter_csv and report_formatter_html.
 * Written by Igor Zhbanov <i.zhbanov@samsung.com>
 * 2012.10 */

#ifndef _REPORT_FORMATTER_BASE_H_
#define _REPORT_FORMATTER_BASE_H_

#include "report-formatter.h"

class report_formatter_string_base: public report_formatter
{
public:
	virtual const char *get_result();
	virtual void clear_result();

	virtual void add(const char *str);
	virtual void addv(const char *fmt, va_list ap);

protected:
	void add_exact(const char *str);
	void addf(const char *fmt, ...)
				__attribute__ ((format (printf, 2, 3)));
	void addf_exact(const char *fmt, ...)
				__attribute__ ((format (printf, 2, 3)));

	virtual std::string escape_string(const char *str) = 0;

	std::string result;
};

#endif /* _REPORT_FORMATTER_BASE_H_ */
