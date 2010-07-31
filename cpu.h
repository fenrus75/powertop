#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>

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

	virtual void	display(void) { cout << "FOO\n"; }
};

class cpu_linux: public abstract_cpu 
{
protected:
	uint64_t cstate_usage[16];
	uint64_t cstate_duration[16];
	uint64_t cstate_usage_after[16];
	uint64_t cstate_duration_after[16];

public:
	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
	virtual void	display(void);
};

class cpu_core: public abstract_cpu 
{
public:
	virtual void	display(void);
};

class cpu_package: public abstract_cpu 
{
public:
	virtual void	display(void);
};

#include "intel_cpus.h"

extern void enumerate_cpus(void);
extern void display_cpus(void);
