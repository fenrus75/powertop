#include <stdio.h>
#include "cpu.h"

void cpu_core::display(void)
{
	unsigned int i;

	cout << "\tCore number " << number << "\n";

	i = 0;
	while (i < this->children.size()) {
		if (this->children[i])
			this->children[i]->display();

		i++;
	}

	for (i = 0; i < states.size(); i++) {
		cout << "\t\t " << states[i]->human_name << "  for " << states[i]->duration_delta / 1000000.0 << "s \n";
	}
}

char * cpu_core::fill_line(int line_nr, char *buffer) 
{
	unsigned int i;
	buffer[0] = 0;

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,"Core %i", number);
		return buffer;
	}

	for (i = 0; i < states.size(); i++) {
		if (states[i]->line_level != line_nr)
			continue;
		sprintf(buffer,"%4.2f %s", states[i]->duration_delta / 1000000.0, states[i]->human_name);
	}

	return buffer; 
}

