#include <stdio.h>
#include "cpu.h"
#include "lib.h"

char * cpu_core::fill_line(int line_nr, char *buffer) 
{
	unsigned int i;
	buffer[0] = 0;

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,"Core");
		return buffer;
	}

	for (i = 0; i < states.size(); i++) {
		if (states[i]->line_level != line_nr)
			continue;
		sprintf(buffer,"%4.1f%% %s", percentage(states[i]->duration_delta / time_factor), states[i]->human_name);
	}

	return buffer; 
}

