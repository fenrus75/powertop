#include <iostream>
#include <fstream>
#include <iomanip>

#include "parameters.h"

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
		file << "\n";
	}

	file.close();

}
