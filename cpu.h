#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>

using namespace std;

class abstract_cpu;

struct power_state {
	char linux_name[16]; /* state0 etc.. cpuidle name */
	char human_name[32];

	uint64_t usage_before;
	uint64_t usage_after;
	uint64_t usage_delta;

	uint64_t duration_before;
	uint64_t duration_after;
	uint64_t duration_delta;

	int before_count;
	int after_count;

	int line_level;
};

class abstract_cpu 
{
protected:
	int	number;
	int	first_cpu;
public:
	vector<class abstract_cpu *> children;
	vector<struct power_state *> states;

	void		set_number(int number, int cpu) {this->number = number; this->first_cpu = cpu;};

	void		insert_state(const char *linux_name, const char *human_name, uint64_t usage, uint64_t duration, int count);
	void		update_state(const char *linux_name, const char *human_name, uint64_t usage, uint64_t duration, int count);
	void		finalize_state(const char *linux_name, uint64_t usage, uint64_t duration, int count);

	virtual int	has_state_level(void);

	virtual void 	measurement_start(void);
	virtual void 	measurement_end(void);
	virtual void	display(void);

};

class cpu_linux: public abstract_cpu 
{
public:
	virtual void 	measurement_start(void);
	virtual void 	measurement_end(void);
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
void start_cpu_measurement(void);
void end_cpu_measurement(void);
