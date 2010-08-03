#include "cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>


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

	aperf_before = get_msr(first_cpu, MSR_APERF);
	mperf_before = get_msr(first_cpu, MSR_MPERF);
	c3_before    = get_msr(first_cpu, MSR_CORE_C3_RESIDENCY);
	c6_before    = get_msr(first_cpu, MSR_CORE_C6_RESIDENCY);
	tsc_before   = get_msr(first_cpu, MSR_TSC);

	gettimeofday(&stamp_before, NULL);

	insert_state("core c3", "C3 (cc3)", 0, c3_before, 1);
	insert_state("core c6", "C6 (cc6)", 0, c6_before, 1);
}

void nhm_core::measurement_end(void)
{
	unsigned int i;
	uint64_t time_delta;
	double ratio;



	aperf_after = get_msr(first_cpu, MSR_APERF);
	mperf_after = get_msr(first_cpu, MSR_MPERF);
	c3_after    = get_msr(first_cpu, MSR_CORE_C3_RESIDENCY);
	c6_after    = get_msr(first_cpu, MSR_CORE_C6_RESIDENCY);
	tsc_after   = get_msr(first_cpu, MSR_TSC);

	gettimeofday(&stamp_after, NULL);

	finalize_state("core c3", 0, c3_after, 1);
	finalize_state("core c6", 0, c6_after, 1);

	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->measurement_end();

	time_delta = 1000000 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;

	ratio = 1.0 * time_delta / (tsc_after - tsc_before);

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

		state->usage_delta =    ratio * (state->usage_after    - state->usage_before)    / state->after_count;
		state->duration_delta = ratio * (state->duration_after - state->duration_before) / state->after_count;
	}
}

void nhm_package::measurement_start(void)
{
	abstract_cpu::measurement_start();

	aperf_before = get_msr(number, MSR_APERF);
	mperf_before = get_msr(number, MSR_MPERF);
	c3_before    = get_msr(number, MSR_PKG_C3_RESIDENCY);
	c6_before    = get_msr(number, MSR_PKG_C6_RESIDENCY);
	tsc_before   = get_msr(first_cpu, MSR_TSC);
	gettimeofday(&stamp_after, NULL);

	insert_state("pkg c3", "C3 (pc3)", 0, c3_before, 1);
	insert_state("pkg c6", "C6 (pc6)", 0, c6_before, 1);
}

void nhm_package::measurement_end(void)
{
	uint64_t time_delta;
	double ratio;
	unsigned int i;


	aperf_after = get_msr(number, MSR_APERF);
	mperf_after = get_msr(number, MSR_MPERF);
	c3_after    = get_msr(number, MSR_PKG_C3_RESIDENCY);
	c6_after    = get_msr(number, MSR_PKG_C6_RESIDENCY);
	tsc_after   = get_msr(first_cpu, MSR_TSC);

	gettimeofday(&stamp_after, NULL);



	finalize_state("pkg c3", 0, c3_after, 1);
	finalize_state("pkg c6", 0, c6_after, 1);

	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->measurement_end();

	time_delta = 1000000 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;

	ratio = 1.0 * time_delta / (tsc_after - tsc_before);


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

		state->usage_delta =    ratio * (state->usage_after    - state->usage_before)    / state->after_count;
		state->duration_delta = ratio * (state->duration_after - state->duration_before) / state->after_count;
	}
}

