#include <iostream>
#include <fstream>

#include "cpu.h"
#include "lib.h"


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


		update_state(linux_name, human_name, usage, duration, 1);		

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


		finalize_state(linux_name, usage, duration, 1);		

	}
	closedir(dir);


	abstract_cpu::measurement_end();
}


void cpu_linux::display(void)
{
	unsigned int i;
	cout << "\t\tCPU number " << number << "\n";

	for (i = 0; i < states.size(); i++) {
		cout << "\t\t\t " << states[i]->human_name << "  for " << states[i]->duration_delta / 1000000.0 << "s \n";
	}
}


char * cpu_linux::fill_line(int line_nr, char *buffer) 
{
	unsigned int i;
	buffer[0] = 0;

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,"CPU %i", number);
		return buffer;
	}

	for (i = 0; i < states.size(); i++) {
		if (states[i]->line_level != line_nr)
			continue;
		sprintf(buffer,"%4.2f %s", states[i]->duration_delta / 1000000.0, states[i]->human_name);
	}

	return buffer; 
}

