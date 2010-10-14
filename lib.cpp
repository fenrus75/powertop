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
#include <map>
#include <string.h>
#include <iostream>
#include <utility>
#include <iostream>
#include <fstream>
#include <string>


#include "lib.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <locale.h>
#include <libintl.h>


static int kallsyms_read = 0;

double percentage(double F)
{
	F = F * 100.0;
//	if (F > 100.0)
//		F = 100.0;
	if (F < 0.0)
		F = 0.0;
	return F;
}

char *hz_to_human(unsigned long hz, char *buffer, int digits)
{
	unsigned long long Hz;

	buffer[0] = 0;

	Hz = hz;

	/* default: just put the Number in */
	sprintf(buffer,_("%9lli"), Hz);

	if (Hz>1000) {
		if (digits == 2)
			sprintf(buffer, _("%4lli Mhz"), (Hz+500)/1000);
		else
			sprintf(buffer, _("%6lli Mhz"), (Hz+500)/1000);
	}

	if (Hz>1500000) {
		if (digits == 2)
			sprintf(buffer, _("%4.2f Ghz"), (Hz+5000.0)/1000000);
		else
			sprintf(buffer, _("%3.1f Ghz"), (Hz+5000.0)/1000000);
	}

	return buffer;
}

using namespace std;

map<unsigned long, string> kallsyms;

static void read_kallsyms(void)
{
	ifstream file;
	char line[1024];
	kallsyms_read = 1;

	file.open("/proc/kallsyms", ios::in);

	while (file) {
		char *c = NULL, *c2 = NULL;
		unsigned long address = 0;
		memset(line, 0, 1024);
		file.getline(line, 1024);
		c = strchr(line, ' ');
		if (!c)
			continue;
		*c = 0;
		c2 = c + 1;
		if (*c2) c2++;
		if (*c2) c2++;

		address = strtoull(line, NULL, 16);
		c = strchr(c2, '\t');
		if (c) 
			*c = 0;
		if (address != 0)
			kallsyms[address] = c2;
	}	
	file.close();
}

const char *kernel_function(uint64_t address)
{
	const char *c;
	if (!kallsyms_read)
		read_kallsyms();

	c = kallsyms[address].c_str();
	if (!c)
		return "";
	return c;
}

static int _max_cpu;
int get_max_cpu(void)
{
	return _max_cpu;
}

void set_max_cpu(int cpu)
{
	if (cpu > _max_cpu)
		_max_cpu = cpu;
}


bool stringless::operator()(const char * const & lhs, const char * const & rhs) const  
{  
	if (strcmp(lhs, rhs) < 0)
		return true;
	return false;
}  

void write_sysfs(string filename, string value)
{
	ofstream file;

	file.open(filename.c_str(), ios::out);
	if (!file)
		return;
	file << value;
	file.close();
}

int read_sysfs(string filename)
{
	ifstream file;
	int i;

	file.open(filename.c_str(), ios::in);
	if (!file)
		return 0;
	file >> i;
	file.close();
	return i;
}


void format_watts(double W, char *buffer, unsigned int len)
{
	buffer[0] = 0;

	if (W > 1.5) 
		sprintf(buffer, "%6.1f   W", W);
	else if (W > 0.5)
		sprintf(buffer, "%7.2f  W", W);
	else if (W > 0.01)
		sprintf(buffer, "%6.1f  mW", W*1000);
	else if (W > 0.001)
		sprintf(buffer, "%7.2f mW", W*1000);
	else if (W > 0.0001)
		sprintf(buffer, "%8.3fmW", W*1000);
	else
		sprintf(buffer, "   0.0  mW");
		
	while (strlen(buffer) < len)
		strcat(buffer, " ");	
}
