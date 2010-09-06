#include <iostream>
#include <fstream>
#include <iomanip>

#include "parameters.h"
#include "../measurement/measurement.h"

using namespace std;

void save_all_results(const char *filename)
{
	ofstream file;
	unsigned int i;
	struct result_bundle *bundle;

	file.open(filename, ios::out);
	if (!file) {
		cout << "Cannot save to file " << filename << "\n";
		return;
	}
	for (i = 0; i < past_results.size(); i++) {	
		bundle = past_results[i];
		map<string, double>::iterator it;
		file << setiosflags(ios::fixed) <<  setprecision(5) << bundle->power << "\n";

		for (it = bundle->utilization.begin(); it != bundle->utilization.end(); it++) {
			file << it->first << "\t" << setprecision(5) << it->second << "\n";
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

	file.open(filename, ios::in);
	if (!file) {
		cout << "Cannot load from file " << filename << "\n";
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
			past_results.push_back(bundle);
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
		bundle->utilization[line] =d;
	}

	file.close();
	printf("Loaded %i prior measurements\n", count);
}

void save_parameters(const char *filename)
{
	ofstream file;

	file.open(filename, ios::out);
	if (!file) {
		cout << "Cannot save to file " << filename << "\n";
		return;
	}
	
	map<string, double>::iterator it;

	for (it = all_parameters.parameters.begin(); it != all_parameters.parameters.end(); it++) {
		file << it->first << "\t" << setprecision(5) << it->second << "\n";
	}	
	file.close();
}

void load_parameters(const char *filename)
{
	ifstream file;
	char line[4096];
	char *c1;

	file.open(filename, ios::in);
	if (!file) {
		cout << "Cannot load from file " << filename << "\n";
		return;
	}

	while (file) {
		double d;
		file.getline(line, 4096);

		c1 = strchr(line, '\t');
		if (!c1)
			continue;
		*c1 = 0;
		c1++;
		sscanf(c1, "%lf", &d);
		
		all_parameters.parameters[line] =d;
	}

	file.close();
}
