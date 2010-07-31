
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