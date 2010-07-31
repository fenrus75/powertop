#include <iostream>
#include <vector>
#include <string>

using namespace std;

class abstract_cpu;

class abstract_cpu 
{
protected:
	int	number;
public:
	vector<class abstract_cpu *> children;

	void	set_number(int number) {this->number = number;};

	void 	measurement_start(void) { };
	void 	measurement_end(void) { };

	void 	consolidate_children(void) { };

	void	display(void) { }
};

class cpu_linux: public abstract_cpu 
{
public:
	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
	void	display(void);
};

class cpu_core: public abstract_cpu 
{
public:
	void	display(void);
};

class cpu_package: public abstract_cpu 
{
public:
	void	display(void);
};

#include "intel_cpus.h"

extern void enumerate_cpus(void);