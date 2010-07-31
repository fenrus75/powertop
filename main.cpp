#include <iostream>

#include "cpu.h"


int main(int argc, char **argv)
{
	enumerate_cpus();

	display_cpus();

	return 0;
}