
#include "cpu.h"

void cpu_package::display(void)
{
	unsigned int i;
	cout << "Package number " << number << "\n";
	
	i = 0;
	while (i < this->children.size()) {
		if (this->children[i])
			this->children[i]->display();

		i++;
	}

	for (i = 0; i < states.size(); i++) {
		cout << "\t " << states[i]->human_name << "  for " << states[i]->duration_delta / 1000000.0 << "s \n";
	}
}

