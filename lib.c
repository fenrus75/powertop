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

void read_cstate_data(int cpu, uint64_t * usage, uint64_t * duration, char **cnames)
{
	DIR *dir;
	struct dirent *entry;
	FILE *file = NULL;
	char line[4096];
	char filename[128], *f;
	int len, clevel = 0;

	memset(usage, 0, 64);
	memset(duration, 0, 64);

	printf("Got here for cpu %i\n", cpu);

	len = sprintf(filename, "/sys/devices/system/cpu/cpu%i/cpuidle", cpu);

	dir = opendir(filename);
	if (!dir)
		return;

	clevel = 0;

	/* For each C-state, there is a stateX directory which
	 * contains a 'usage' and a 'time' (duration) file */
	while ((entry = readdir(dir))) {
		if (strlen(entry->d_name) < 3)
			continue;

		sprintf(filename + len, "/%s/desc", entry->d_name);
		if (cnames) 
			file = fopen(filename, "r");
		else
			file = NULL;
		if (file) {
			memset(line, 0, 4096);
			f = fgets(line, 4096, file);
			fclose(file);
			if (f == NULL)
				break;


			f = strstr(line, "MWAIT ");
			if (f) {
				f += 6;
				clevel = (strtoull(f, NULL, 16)>>4) + 1;
				sprintf(cnames[clevel], "C%i mwait", clevel);
			} else
				sprintf(cnames[clevel], "C%i\t", clevel);

			f = strstr(line, "POLL IDLE");
			if (f) {
				clevel = 0;
				sprintf(cnames[clevel], "%s\t", _("polling"));
			}

			f = strstr(line, "ACPI HLT");
			if (f) {
				clevel = 1;
				sprintf(cnames[clevel], "%s\t", "C1 halt");
			}
		}
		sprintf(filename + len, "/%s/usage", entry->d_name);
		file = fopen(filename, "r");
		if (!file)
			continue;

		memset(line, 0, 4096);
		f = fgets(line, 4096, file);
		fclose(file);
		if (f == NULL)
			break;

		usage[clevel] += 1+strtoull(line, NULL, 10);

		sprintf(filename + len, "/%s/time", entry->d_name);
		file = fopen(filename, "r");
		if (!file)
			continue;

		memset(line, 0, 4096);
		f = fgets(line, 4096, file);
		fclose(file);
		if (f == NULL)
			break;

		duration[clevel] += 1+strtoull(line, NULL, 10);

		clevel++;

	}
	closedir(dir);

}

