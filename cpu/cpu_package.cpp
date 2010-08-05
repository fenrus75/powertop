#include <stdio.h>
#include "cpu.h"
#include "../lib.h"

char * cpu_package::fill_line(int line_nr, char *buffer) 
{
	unsigned int i;
	buffer[0] = 0;

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,"Package %i", number);
		return buffer;
	}

	for (i = 0; i < states.size(); i++) {
		if (states[i]->line_level != line_nr)
			continue;

		sprintf(buffer,"%5.1f%%", percentage(states[i]->duration_delta / time_factor));
	}

	return buffer; 
}


char * cpu_package::fill_state_name(int line_nr, char *buffer) 
{
	unsigned int i;
	buffer[0] = 0;

	for (i = 0; i < states.size(); i++) {
		if (states[i]->line_level != line_nr)
			continue;

		sprintf(buffer,"%s", states[i]->human_name);
	}

	return buffer; 
}

