#include <stdio.h>
#include "cpu.h"
#include "../lib.h"

char * cpu_core::fill_cstate_line(int line_nr, char *buffer) 
{
	unsigned int i;
	buffer[0] = 0;

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,"  Core");
		return buffer;
	}

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;
		sprintf(buffer,"%5.1f%%", percentage(cstates[i]->duration_delta / time_factor));
	}

	return buffer; 
}


char * cpu_core::fill_cstate_name(int line_nr, char *buffer) 
{
	unsigned int i;
	buffer[0] = 0;

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;

		sprintf(buffer,"%s", cstates[i]->human_name);
	}

	return buffer; 
}

