
#include "cpu.h"

void cpu_core::display(void)
{
	unsigned int i;

	cout << "\tCore number " << number << "\n";

	i = 0;
	while (i < this->children.size()) {
		if (this->children[i])
			this->children[i]->display();

		i++;
	}

}

void cpu_core::measurement_start(void)
{
	unsigned int i;
	i = 0;
	while (i < this->children.size()) {
		if (this->children[i])
			this->children[i]->measurement_start();

		i++;
	}
}

void cpu_core::measurement_end(void)
{
	unsigned int i;
	i = 0;
	while (i < this->children.size()) {
		if (this->children[i])
			this->children[i]->measurement_end();

		i++;
	}
}
