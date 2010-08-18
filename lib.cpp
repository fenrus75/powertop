#include <map>
#include <string.h>
#include <iostream>
#include <utility>
#include <iostream>
#include <fstream>


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


#include "lib.h"


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

	if (Hz>1000)
		sprintf(buffer, _("%6lli Mhz"), (Hz+500)/1000);

	if (Hz>1500000) {
		if (digits == 2)
			sprintf(buffer, _("%4.2f Ghz"), (Hz+5000.0)/1000000);
		else
			sprintf(buffer, _("%3.1f Ghz"), (Hz+5000.0)/1000000);
	}

	return buffer;
}

using namespace std;

map<unsigned long, const char *> kallsyms;

static void read_kallsyms(void)
{
	ifstream file;
	char line[1024];
	kallsyms_read = 1;

	file.open("/proc/kallsyms", ios::in);

	while (file) {
		char *c, *c2;
		unsigned long address;
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
		kallsyms[address] = strdup(c2);
	}	
	file.close();
}

const char *kernel_function(uint64_t address)
{
	const char *c;
	if (!kallsyms_read)
		read_kallsyms();

	c = kallsyms[address];
	if (!c)
		return "";
	return c;
}