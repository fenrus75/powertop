#include <iostream>
#include <vector>
#include <string>

using namespace std;

class abstract_cpu;

class abstract_cpu 
{
public:
	vector<class abstract_cpu> children;

	void 	measurement_start(void) { };
	void 	measurement_end(void) { };

	void 	consolidate_children(void) { };
};

class cpu_linux: public abstract_cpu 
{
private:
	int	number;
public:
	cpu_linux(int number);

	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
};

class cpu_core: public abstract_cpu 
{
};

class cpu_package: public abstract_cpu 
{
};


extern void enumerate_cpus(void);
