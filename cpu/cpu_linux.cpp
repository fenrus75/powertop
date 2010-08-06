#include <iostream>
#include <fstream>

#include "cpu.h"
#include "../lib.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>


void cpu_linux::measurement_start(void)
{
	abstract_cpu::measurement_start();

	DIR *dir;
	struct dirent *entry;
	char filename[128];
	int len;

	len = sprintf(filename, "/sys/devices/system/cpu/cpu%i/cpuidle", number);

	dir = opendir(filename);
	if (!dir)
		return;

	/* For each C-state, there is a stateX directory which
	 * contains a 'usage' and a 'time' (duration) file */
	while ((entry = readdir(dir))) {
		ifstream file; 
		char linux_name[64];
		char human_name[64];
		uint64_t usage = 0;
		uint64_t duration = 0;


		if (strlen(entry->d_name) < 3)
			continue;

		strcpy(linux_name, entry->d_name);
		strcpy(human_name, linux_name);

		sprintf(filename + len, "/%s/name", entry->d_name);

		file.open(filename, ios::in);
		if (file) {
			file.getline(human_name, 64);
			file.close();
		}

		if (strcmp(human_name, "C0")==0)
			strcpy(human_name, "C0 polling");

		sprintf(filename + len, "/%s/usage", entry->d_name);
		file.open(filename, ios::in); 
		if (file) {
			file >> usage;
			file.close();
		}

		sprintf(filename + len, "/%s/time", entry->d_name);

		file.open(filename, ios::in);
		if (file) {
			file >> duration;
			file.close();
		}


		update_cstate(linux_name, human_name, usage, duration, 1);		

	}
	closedir(dir);

}


void cpu_linux::measurement_end(void)
{
	DIR *dir;
	struct dirent *entry;
	char filename[128];
	int len;

	len = sprintf(filename, "/sys/devices/system/cpu/cpu%i/cpuidle", number);

	dir = opendir(filename);
	if (!dir)
		return;

	/* For each C-state, there is a stateX directory which
	 * contains a 'usage' and a 'time' (duration) file */
	while ((entry = readdir(dir))) {
		ifstream file; 
		char linux_name[64];
		char human_name[64];
		uint64_t usage = 0;
		uint64_t duration = 0;


		if (strlen(entry->d_name) < 3)
			continue;

		strcpy(linux_name, entry->d_name);
		strcpy(human_name, linux_name);


		sprintf(filename + len, "/%s/usage", entry->d_name);
		file.open(filename, ios::in); 
		if (file) {
			file >> usage;
			file.close();
		}

		sprintf(filename + len, "/%s/time", entry->d_name);

		file.open(filename, ios::in);
		if (file) {
			file >> duration;
			file.close();
		}


		finalize_cstate(linux_name, usage, duration, 1);		

	}
	closedir(dir);


	abstract_cpu::measurement_end();
}


char * cpu_linux::fill_cstate_line(int line_nr, char *buffer) 
{
	unsigned int i;
	buffer[0] = 0;

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,"  CPU %i", number);
		return buffer;
	}

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;

		sprintf(buffer,"%5.1f%% %6.1f ms", percentage(cstates[i]->duration_delta / time_factor), 1.0 * cstates[i]->duration_delta / (1+cstates[i]->usage_delta) / 1000);
	}

	return buffer; 
}

char * cpu_linux::fill_cstate_name(int line_nr, char *buffer) 
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

