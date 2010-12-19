/*
 * Copyright 2010, Intel Corporation
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
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */

#include "html.h"
#include <errno.h>
#include <string.h>

using namespace std;


FILE *htmlout;

static void css_header(void)
{
	if (!htmlout)
		return;

	fprintf(htmlout, "<link rel=\"stylesheet\" href=\"powertop.css\">\n");
}

void init_html_output(const char *filename)
{
	htmlout = fopen(filename, "wm");

	if (!htmlout) {
		fprintf(stderr, "Cannot open output file %s (%s)\n", filename, strerror(errno));
	}

	fprintf(htmlout, "<!DOCTYPE html PUBLIC \"-//W3C/DTD HTML 4.01//EN\">\n");
	fprintf(htmlout, "<html>\n\n");
	fprintf(htmlout, "<head>\n");
	
	css_header();

	fprintf(htmlout, "</head>\n\n");
	fprintf(htmlout, "<body>\n");
}

void finish_html_output(void)
{
	if (!htmlout)
		return;

	fprintf(htmlout, "</body>\n\n");
	fprintf(htmlout, "</html>\n");
}
