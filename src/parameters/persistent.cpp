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
#include <cstdlib>

#include "parameters.h"
#include "../measurement/measurement.h"

void save_all_results(const std::string &filename)
{
	std::ofstream file;
	const std::string pathname = get_param_directory(filename);

	file.open(pathname, std::ios::out);
	if (!file) {
		fprintf(stderr, "%s %s\n", _("Cannot save to file"), pathname.c_str());
		return;
	}
	for (auto *bundle : past_results) {
		std::map<std::string, int>::iterator it;
		file << std::setiosflags(std::ios::fixed) <<  std::setprecision(5) << bundle->power << "\n";

		for (it = result_index.begin(); it != result_index.end(); it++) {
			file << it->first << "\t" << std::setprecision(5) << get_result_value(it->second, bundle) << "\n";
		}
		file << ":\n";
	}

	file.close();

}

void close_results()
{
	for (auto *r : past_results)
		delete r;

	past_results.clear();
	return;
}

void load_results(const std::string &filename)
{
	std::string line;
	struct result_bundle *bundle;
	int first = 1;
	unsigned int count = 0;
	const std::string pathname = get_param_directory(filename);
	int bundle_saved = 0;

	if (pathname.empty() || pt_access(pathname, R_OK) != 0)
		return;

	const std::string content = read_file_content(pathname);
	if (content.empty()) {
		fprintf(stderr, "%s %s\n", _("Cannot load from file"), pathname.c_str());
		return;
	}

	std::istringstream stream(content);

	bundle = new struct result_bundle;

	while (getline(stream, line)) {
		double d;
		if (first) {
			if (!line.empty()) {
				try {
					bundle->power = std::stod(line);
					if (bundle->power < min_power)
						min_power = bundle->power;
				} catch (...) {}
			}
			first = 0;
			continue;
		}

		if (line.length() < 3) {
			bundle_saved = 1;
			const int overflow_index = 50 + (rand() % MAX_KEEP);
			if (past_results.size() >= MAX_PARAM) {
				delete past_results[overflow_index];
				past_results[overflow_index] = bundle;
			} else {
				past_results.push_back(bundle);
			}
			bundle = new struct result_bundle;
			first = 1;
			count++;
			continue;
		}

		const size_t pos = line.find('\t');
		if (pos == std::string::npos)
			continue;

		const std::string name = line.substr(0, pos);
		const std::string value_str = line.substr(pos + 1);
		try {
			d = std::stod(value_str);
			set_result_value(name, d, bundle);
		} catch (...) {}
	}

	delete bundle;   /* always discard last-allocated but unsaved bundle */

	if (bundle_saved == 0)
		return;

	// '%i" is for count, do not translate
	fprintf(stderr, _("Loaded %i prior measurements\n"), (int)count);
}

void save_parameters(const std::string &filename)
{
	std::ofstream file;
	const std::string pathname = get_param_directory(filename);

	file.open(pathname, std::ios::out);
	if (!file) {
		fprintf(stderr, "%s %s\n", _("Cannot save to file"), pathname.c_str());
		return;
	}

	std::map<std::string, int>::iterator it;

	for (it = param_index.begin(); it != param_index.end(); it++) {
		const int index = it->second;
		file << it->first << "\t" << std::setprecision(9) << all_parameters.parameters[index] << "\n";
	}
	file.close();
}

void load_parameters(const std::string &filename)
{
	std::string line;
	const std::string pathname = get_param_directory(filename);

	if (pathname.empty() || pt_access(pathname, R_OK) != 0)
		return;

	const std::string content = read_file_content(pathname);
	if (content.empty()) {
		fprintf(stderr, "%s %s\n", _("Cannot load from file"), pathname.c_str());
		return;
	}

	std::istringstream stream(content);

	while (getline(stream, line)) {
		double d;
		const size_t pos = line.find('\t');
		if (pos == std::string::npos)
			continue;

		const std::string name = line.substr(0, pos);
		const std::string value_str = line.substr(pos + 1);
		try {
			d = std::stod(value_str);
			set_parameter_value(name, d);
		} catch (...) {}
	}
}
