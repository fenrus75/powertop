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
	if (F > 100.0)
		F = 100.0;
	if (F < 0.0)
		F = 0.0;
	return F;
}