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