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
 *	Peter Anvin
 */
#include <map>
#include <string.h>
#include <iostream>
#include <utility>
#include <iostream>
#include <fstream>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

extern "C" {
#include <pci/pci.h>
}

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
	char buf[32];

#if 0
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
#endif
	sprintf(buffer, "%7sW", fmt_prefix(W, buf));

	if (W < 0.0001)
		sprintf(buffer, "    0 mW");
		
			
	while (strlen(buffer) < len)
		strcat(buffer, " ");	
}


static struct pci_access *pci_access;

char *pci_id_to_name(uint16_t vendor, uint16_t device, char *buffer, int len)
{
	char *ret;

	buffer[0] = 0;

	if (!pci_access) {
		pci_access = pci_alloc();
		pci_init(pci_access);
	}

	ret = pci_lookup_name(pci_access, buffer, len, PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE, vendor, device);

	return ret;
}

static int utf_ok = -1;



/* pretty print numbers while limiting the precision */
char *fmt_prefix(double n, char *buf)
{
	static const char prefixes[] = "yzafpnum kMGTPEZY";
	char tmpbuf[16];
	int omag, npfx;
	char *p, *q, pfx;
	int i;

	if (utf_ok == -1) {
		if (strstr(getenv("LANG"), "UTF-8"))
			utf_ok = 1;
		else
			utf_ok = 0; 
	}

	p = buf;

	*p = ' ';
	if (n < 0.0) {
		*p = '-';
		n = -n;
	}
	p++;

	snprintf(tmpbuf, sizeof tmpbuf, "%.2e", n);
	omag = atoi(strchr(tmpbuf, 'e') + 1);

	npfx = ((omag + 27) / 3) - (27/3);
	omag = (omag + 27) % 3;

	q = tmpbuf;
	if (omag == 2)
		omag = -1;

	for (i = 0; i < 3; i++) {
		while (!isdigit(*q))
			q++;
		*p++ = *q++;
		if (i == omag)
			*p++ = '.';
	}
	*p++ = ' ';

	pfx = prefixes[npfx + 8];

	if (pfx == ' ') {
		/* do nothing */
	} else if (pfx == 'u' && utf_ok > 0) {
		strcpy(p, "µ");		/* Mu is a multibyte sequence */
		while (*p)
			p++;
	} else {
		*p++ = pfx;
	}
	*p = '\0';

	return buf;
}

