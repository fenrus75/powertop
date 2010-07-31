
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
}

void cpu_package::measurement_start(void)
{
	unsigned int i;
	i = 0;
	while (i < this->children.size()) {
		if (this->children[i])
			this->children[i]->measurement_start();

		i++;
	}
}

void cpu_package::measurement_end(void)
{
	unsigned int i;
	i = 0;
	while (i < this->children.size()) {
		if (this->children[i])
			this->children[i]->measurement_end();

		i++;
	}
}
