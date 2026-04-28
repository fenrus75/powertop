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
#include <sstream>
#include <iomanip>
#include <stdlib.h>

#include "parameters.h"
#include "../measurement/measurement.h"

void save_all_results(const std::string &filename)
{
	ofstream file;
	unsigned int i;
	struct result_bundle *bundle;
	std::string pathname;

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

void close_results()
{
	for (unsigned int i = 0; i < past_results.size(); i++) {
		delete past_results[i];
	}

	past_results.clear();
	return;
}

void load_results(const std::string &filename)
{
	string line;
	struct result_bundle *bundle;
	int first = 1;
	unsigned int count = 0;
	std::string pathname;
	int bundle_saved = 0;

	pathname = get_param_directory(filename);

	std::string content = read_file_content(pathname);
	if (content.empty()) {
		cout << _("Cannot load from file") << " " << pathname << "\n";
		return;
	}

	std::istringstream stream(content);

	bundle = new struct result_bundle;

	while (getline(stream, line)) {
		double d;
		if (first) {
			if (line.length() > 0) {
				try {
					bundle->power = stod(line);
					if (bundle->power < min_power)
						min_power = bundle->power;
				} catch (...) {}
			}
			first = 0;
			continue;
		}

		if (line.length() < 3) {
			int overflow_index;

			bundle_saved = 1;
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

		size_t pos = line.find('\t');
		if (pos == string::npos)
			continue;

		string name = line.substr(0, pos);
		string value_str = line.substr(pos + 1);
		try {
			d = stod(value_str);
			set_result_value(name, d, bundle);
		} catch (...) {}
	}

	if (bundle_saved == 0)
		delete bundle;

	// '%i" is for count, do not translate
	fprintf(stderr, _("Loaded %i prior measurements\n"), (int)count);
}

void save_parameters(const std::string &filename)
{
	ofstream file;
	std::string pathname;

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

void load_parameters(const std::string &filename)
{
	string line;
	std::string pathname;

	pathname = get_param_directory(filename);

	std::string content = read_file_content(pathname);
	if (content.empty()) {
		cout << _("Cannot load from file") << " " << pathname << "\n";
		cout << _("File will be loaded after taking minimum number of measurement(s) with battery only \n");
		return;
	}

	std::istringstream stream(content);

	while (getline(stream, line)) {
		double d;
		size_t pos = line.find('\t');
		if (pos == string::npos)
			continue;

		string name = line.substr(0, pos);
		string value_str = line.substr(pos + 1);
		try {
			d = stod(value_str);
			set_parameter_value(name, d);
		} catch (...) {}
	}
}
