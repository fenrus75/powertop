#include <stdio.h>
#include "cpu.h"
#include "../lib.h"
#include "../parameters/parameters.h"

char * cpu_package::fill_cstate_line(int line_nr, char *buffer) 
{
	unsigned int i;
	buffer[0] = 0;

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,"Package");
		return buffer;
	}

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;

		sprintf(buffer,"%5.1f%%", percentage(cstates[i]->duration_delta / time_factor));
	}

	return buffer; 
}


char * cpu_package::fill_cstate_name(int line_nr, char *buffer) 
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



char * cpu_package::fill_pstate_name(int line_nr, char *buffer) 
{
	buffer[0] = 0;

	if (line_nr >= (int)pstates.size() || line_nr < 0)
		return buffer;

	sprintf(buffer,"%s", pstates[line_nr]->human_name);

	return buffer; 
}

char * cpu_package::fill_pstate_line(int line_nr, char *buffer) 
{
	buffer[0] = 0;

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,"Package");
		return buffer;
	}

	if (line_nr >= (int)pstates.size() || line_nr < 0)
		return buffer;


	sprintf(buffer," %5.1f%% ", percentage(1.0* (pstates[line_nr]->time_after - pstates[line_nr]->time_before) / time_factor * 10000 / pstates[line_nr]->after_count));
	return buffer; 
}

