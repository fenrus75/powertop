#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>
#include <sys/time.h>

using namespace std;

class abstract_cpu;

#define LEVEL_C0 -1
#define LEVEL_HEADER -2

struct idle_state {
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

struct frequency {
	char human_name[32];
	int line_level;

	uint64_t freq;
	
	uint64_t time_before;
	uint64_t time_after;

	int before_count;
	int after_count;

	double   display_value;	
};

class abstract_cpu 
{
protected:
	int	number;
	int	first_cpu;
	struct timeval	stamp_before, stamp_after;
	double  time_factor;
	uint64_t max_frequency;
	uint64_t max_minus_one_frequency;
public:
	int	childcount;
	vector<class abstract_cpu *> children;
	vector<struct idle_state *> cstates;
	vector<struct frequency *> pstates;

	void		set_number(int number, int cpu) {this->number = number; this->first_cpu = cpu;};

	virtual void 	measurement_start(void);
	virtual void 	measurement_end(void);

	virtual int     can_collapse(void) { return 0;};


	/* C state related methods */

	void		insert_cstate(const char *linux_name, const char *human_name, uint64_t usage, uint64_t duration, int count);
	void		update_cstate(const char *linux_name, const char *human_name, uint64_t usage, uint64_t duration, int count);
	void		finalize_cstate(const char *linux_name, uint64_t usage, uint64_t duration, int count);

	virtual int	has_cstate_level(int level);

	virtual char *  fill_cstate_line(int line_nr, char *buffer) { return buffer;};
	virtual char *  fill_cstate_name(int line_nr, char *buffer) { return buffer;};


	/* P state related methods */
	void		insert_pstate(uint64_t freq, const char *human_name, uint64_t duration, int count);
	void		update_pstate(uint64_t freq, const char *human_name, uint64_t duration, int count);
	void		finalize_pstate(uint64_t freq, uint64_t duration, int count);


	virtual char *  fill_pstate_line(int line_nr, char *buffer) { return buffer;};
	virtual char *  fill_pstate_name(int line_nr, char *buffer) { return buffer;};

};

class cpu_linux: public abstract_cpu 
{
public:
	virtual void 	measurement_start(void);
	virtual void 	measurement_end(void);

	virtual char *  fill_cstate_line(int line_nr, char *buffer);
	virtual char *  fill_cstate_name(int line_nr, char *buffer);

	virtual char *  fill_pstate_line(int line_nr, char *buffer);
	virtual char *  fill_pstate_name(int line_nr, char *buffer);

};

class cpu_core: public abstract_cpu 
{
public:
	virtual char *  fill_cstate_line(int line_nr, char *buffer);
	virtual char *  fill_cstate_name(int line_nr, char *buffer);

	virtual char *  fill_pstate_line(int line_nr, char *buffer);
	virtual char *  fill_pstate_name(int line_nr, char *buffer);

	virtual int     can_collapse(void) { return childcount == 1;};
};

class cpu_package: public abstract_cpu 
{
public:
	virtual char *  fill_cstate_line(int line_nr, char *buffer);
	virtual char *  fill_cstate_name(int line_nr, char *buffer);

	virtual char *  fill_pstate_line(int line_nr, char *buffer);
	virtual char *  fill_pstate_name(int line_nr, char *buffer);
	virtual int     can_collapse(void) { return childcount == 1;};
};

#include "intel_cpus.h"

extern void enumerate_cpus(void);

extern void display_cpu_cstates(void);
extern void display_cpu_pstates(void);

void start_cpu_measurement(void);
void end_cpu_measurement(void);
