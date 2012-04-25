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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv)
{
	FILE *in, *out;
	char line[4096];

	if (argc < 2) {
		printf("Usage:  csstoh cssfile header.h \n");
		exit(0);
	}
	in = fopen(argv[1], "rm");
	if (!in) {
		printf("Failed to open input file %s (%s) \n", argv[1], strerror(errno));
		exit(0);
	}
	out = fopen(argv[2], "wm");
	if (!out) {
		printf("Failed to open output file %s (%s) \n", argv[1], strerror(errno));
		exit(0);
	}

	fprintf(out, "#ifndef __INCLUDE_GUARD_CCS_H\n");
	fprintf(out, "#define __INCLUDE_GUARD_CCS_H\n");
	fprintf(out, "\n");
	fprintf(out, "const char css[] = \n");

	while (!feof(in)) {
		char *c;
		if (fgets(line, 4095, in) == NULL)
			break;
		c = strchr(line, '\n');
		if (c) *c = 0;
		fprintf(out, "\t\"%s\\n\"\n", line);
	}	
	fprintf(out, ";\n");	
	fprintf(out, "#endif\n");
	fclose(out);
	fclose(in);
	return EXIT_SUCCESS;
}
