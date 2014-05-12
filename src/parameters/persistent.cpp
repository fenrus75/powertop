/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>

#include "parameters.h"
#include "../measurement/measurement.h"

using namespace std;

void save_all_results(const char *filename)
{
	ofstream file;
	unsigned int i;
	struct result_bundle *bundle;
	char* pathname;

	pathname = get_param_directory(filename);

	file.open(pathname, ios::out);
	if (!file) {
		cout << _("Cannot save to file") << " " << pathname << "\n";
		return;
	}
	for (i = 0; i < past_results.size(); i++) {
		bundle = past_results[i];
		map<string, int>::iterator it;
		file << setiosflags(ios::fixed) <<  setprecision(5) << bundle->power << "\n";

		for (it = result_index.begin(); it != result_index.end(); it++) {
			file << it->first << "\t" << setprecision(5) << get_result_value(it->second, bundle) << "\n";
		}
		file << ":\n";
	}

	file.close();

}

void load_results(const char *filename)
{
	ifstream file;
	char line[4096];
	char *c1;
	struct result_bundle *bundle;
	int first = 1;
	unsigned int count = 0;
	char* pathname;

	pathname = get_param_directory(filename);

	file.open(pathname, ios::in);
	if (!file) {
		cout << _("Cannot load from file") << " " << pathname << "\n";
		return;
	}

	bundle = new struct result_bundle;

	while (file) {
		double d;
		if (first) {
			file.getline(line, 4096);
			if (strlen(line)>0) {
				sscanf(line, "%lf", &bundle->power);
				if (bundle->power < min_power)
					min_power = bundle->power;
			}
			first = 0;
			continue;
		}
		file.getline(line, 4096);
		if (strlen(line) < 3) {
			int overflow_index;

			overflow_index = 50 + (rand() % MAX_KEEP);
			if (past_results.size() >= MAX_PARAM) {
			/* memory leak, must free old one first */
				past_results[overflow_index] = bundle;
			} else {
				past_results.push_back(bundle);
			}
			bundle = new struct result_bundle;
			first = 1;
			count++;
			continue;
		}
		c1 = strchr(line, '\t');
		if (!c1)
			continue;
		*c1 = 0;
		c1++;
		sscanf(c1, "%lf", &d);
		set_result_value(line, d, bundle);
	}

	file.close();
	// '%i" is for count, do not translate
	fprintf(stderr, _("Loaded %i prior measurements\n"), count);
}

void save_parameters(const char *filename)
{
	ofstream file;
	char* pathname;

//	printf("result size is %i, #parameters is %i \n", (int)past_results.size(), (int)all_parameters.parameters.size());

	if (!global_power_valid())
		return;

	pathname = get_param_directory(filename);

	file.open(pathname, ios::out);
	if (!file) {
		cout << _("Cannot save to file") << " " << pathname << "\n";
		return;
	}

	map<string, int>::iterator it;

	for (it = param_index.begin(); it != param_index.end(); it++) {
		int index;
		index = it->second;
		file << it->first << "\t" << setprecision(9) << all_parameters.parameters[index] << "\n";
	}
	file.close();
}

void load_parameters(const char *filename)
{
	ifstream file;
	char line[4096];
	char *c1;
	char* pathname;

	pathname = get_param_directory(filename);

	file.open(pathname, ios::in);
	if (!file) {
		cout << _("Cannot load from file") << " " << pathname << "\n";
		return;
	}

	while (file) {
		double d;
		memset(line, 0, 4096);
		file.getline(line, 4095);

		c1 = strchr(line, '\t');
		if (!c1)
			continue;
		*c1 = 0;
		c1++;
		sscanf(c1, "%lf", &d);


		set_parameter_value(line, d);
	}

	file.close();
}
