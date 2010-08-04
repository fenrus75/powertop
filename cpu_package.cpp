#include <stdio.h>
#include "cpu.h"

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
		sprintf(buffer,"%4.2f %s", states[i]->duration_delta / 1000000.0, states[i]->human_name);
	}

	return buffer; 
}

