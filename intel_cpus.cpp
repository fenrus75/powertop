#include "cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


static uint64_t get_msr(int cpu, uint64_t offset)
{
	ssize_t retval;
	uint64_t msr;
	int fd;
	char msr_path[256];

	fd  =sprintf(msr_path, "/dev/cpu/%d/msr", cpu);
	fd = open(msr_path, O_RDONLY);

	retval = pread(fd, &msr, sizeof msr, offset);
	if (retval != sizeof msr) {
		fprintf(stderr, "pread cpu%d 0x%llx \n", cpu, offset);
		_exit(-2);
	}
	close(fd);
	return msr;
}

void nhm_core::measurement_start(void)
{
	abstract_cpu::measurement_start();

	aperf_before = get_msr(number, MSR_APERF);
	mperf_before = get_msr(number, MSR_MPERF);
	c3_before    = get_msr(number, MSR_CORE_C3_RESIDENCY);
	c6_before    = get_msr(number, MSR_CORE_C6_RESIDENCY);
}

void nhm_core::measurement_end(void)
{
	unsigned int i;
	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->measurement_end();


	aperf_after = get_msr(number, MSR_APERF);
	mperf_after = get_msr(number, MSR_MPERF);
	c3_after    = get_msr(number, MSR_CORE_C3_RESIDENCY);
	c6_after    = get_msr(number, MSR_CORE_C6_RESIDENCY);




	for (i = 0; i < states.size(); i++) {
		struct power_state *state = states[i];

		if (state->after_count == 0) {
			cout << "after count is 0\n";
			continue;
		}

		if (state->after_count != state->before_count) {
			cout << "count mismatch\n";
			continue;
		}

		state->usage_delta =    (state->usage_after    - state->usage_before)    / state->after_count;
		state->duration_delta = (state->duration_after - state->duration_before) / state->after_count;
	}
}

void nhm_package::measurement_start(void)
{
	abstract_cpu::measurement_start();
}

void nhm_package::measurement_end(void)
{
	unsigned int i;
	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->measurement_end();

	for (i = 0; i < states.size(); i++) {
		struct power_state *state = states[i];

		if (state->after_count == 0) {
			cout << "after count is 0\n";
			continue;
		}

		if (state->after_count != state->before_count) {
			cout << "count mismatch\n";
			continue;
		}

		state->usage_delta =    (state->usage_after    - state->usage_before)    / state->after_count;
		state->duration_delta = (state->duration_after - state->duration_before) / state->after_count;
	}
}

