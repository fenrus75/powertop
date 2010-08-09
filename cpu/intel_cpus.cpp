#include "cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "../lib.h"

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
	/* the abstract function needs to be first since it clears all state */
	abstract_cpu::measurement_start();

	c3_before    = get_msr(first_cpu, MSR_CORE_C3_RESIDENCY);
	c6_before    = get_msr(first_cpu, MSR_CORE_C6_RESIDENCY);
	tsc_before   = get_msr(first_cpu, MSR_TSC);

	insert_cstate("core c3", "C3 (cc3)", 0, c3_before, 1);
	insert_cstate("core c6", "C6 (cc6)", 0, c6_before, 1);
}

void nhm_core::measurement_end(void)
{
	unsigned int i, j;
	uint64_t time_delta;
	double ratio;



	c3_after    = get_msr(first_cpu, MSR_CORE_C3_RESIDENCY);
	c6_after    = get_msr(first_cpu, MSR_CORE_C6_RESIDENCY);
	tsc_after   = get_msr(first_cpu, MSR_TSC);

	finalize_cstate("core c3", 0, c3_after, 1);
	finalize_cstate("core c6", 0, c6_after, 1);

	gettimeofday(&stamp_after, NULL);

	time_factor = 1000000.0 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;



	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->measurement_end();

	time_delta = 1000000 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;

	ratio = 1.0 * time_delta / (tsc_after - tsc_before);

	for (i = 0; i < cstates.size(); i++) {
		struct idle_state *state = cstates[i];

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

	for (i = 0; i < children.size(); i++)
		if (children[i]) {
			for (j = 0; j < children[i]->pstates.size(); j++) {
				struct frequency *state;
				state = children[i]->pstates[j];
				if (!state)
					continue;

				update_pstate(  state->freq, state->human_name, state->time_before, state->before_count);
				finalize_pstate(state->freq,                    state->time_after,  state->after_count);
			}
		}

}

void nhm_package::measurement_start(void)
{
	abstract_cpu::measurement_start();

	c3_before    = get_msr(number, MSR_PKG_C3_RESIDENCY);
	c6_before    = get_msr(number, MSR_PKG_C6_RESIDENCY);
	tsc_before   = get_msr(first_cpu, MSR_TSC);

	insert_cstate("pkg c3", "C3 (pc3)", 0, c3_before, 1);
	insert_cstate("pkg c6", "C6 (pc6)", 0, c6_before, 1);
}

void nhm_package::measurement_end(void)
{
	uint64_t time_delta;
	double ratio;
	unsigned int i, j;


	c3_after    = get_msr(number, MSR_PKG_C3_RESIDENCY);
	c6_after    = get_msr(number, MSR_PKG_C6_RESIDENCY);
	tsc_after   = get_msr(first_cpu, MSR_TSC);

	gettimeofday(&stamp_after, NULL);

	time_factor = 1000000.0 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;


	finalize_cstate("pkg c3", 0, c3_after, 1);
	finalize_cstate("pkg c6", 0, c6_after, 1);

	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->measurement_end();

	time_delta = 1000000 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;

	ratio = 1.0 * time_delta / (tsc_after - tsc_before);


	for (i = 0; i < cstates.size(); i++) {
		struct idle_state *state = cstates[i];

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
	for (i = 0; i < children.size(); i++)
		if (children[i]) {
			for (j = 0; j < children[i]->pstates.size(); j++) {
				struct frequency *state;
				state = children[i]->pstates[j];
				if (!state)
					continue;

				update_pstate(  state->freq, state->human_name, state->time_before, state->before_count);
				finalize_pstate(state->freq,                    state->time_after,  state->after_count);
			}
		}
}


void nhm_cpu::measurement_start(void)
{
	cpu_linux::measurement_start();

	aperf_before = get_msr(number, MSR_APERF);
	mperf_before = get_msr(number, MSR_MPERF);
	tsc_before   = get_msr(number, MSR_TSC);

	insert_cstate("active", "C0 active", 0, aperf_before, 1);
}

void nhm_cpu::measurement_end(void)
{
	uint64_t time_delta;
	double ratio;
	unsigned int i;


	aperf_after = get_msr(number, MSR_APERF);
	mperf_after = get_msr(number, MSR_MPERF);
	tsc_after   = get_msr(number, MSR_TSC);



	finalize_cstate("active", 0, aperf_after, 1);


	cpu_linux::measurement_end();

	time_delta = 1000000 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;

	ratio = 1.0 * time_delta / (tsc_after - tsc_before);


	for (i = 0; i < cstates.size(); i++) {
		struct idle_state *state = cstates[i];
		if (state->line_level != LEVEL_C0)
			continue;

		state->usage_delta =    ratio * (state->usage_after    - state->usage_before)    / state->after_count;
		state->duration_delta = ratio * (state->duration_after - state->duration_before) / state->after_count;
	}
}


char * nhm_cpu::fill_pstate_name(int line_nr, char *buffer)
{
	if (line_nr == LEVEL_C0) {
		sprintf(buffer, "Effective");
		return buffer;
	}
	return cpu_linux::fill_pstate_name(line_nr, buffer);
}

char * nhm_cpu::fill_pstate_line(int line_nr, char *buffer)
{
	if (line_nr == LEVEL_C0) {
		double F;
		F = 1.0 * (tsc_after - tsc_before) * (aperf_after - aperf_before) / (mperf_after - mperf_before) / time_factor * 1000;
		sprintf(buffer, "%s", hz_to_human(F, buffer, 1));
		return buffer;
	}
	return cpu_linux::fill_pstate_line(line_nr, buffer);
}


int nhm_cpu::has_pstate_level(int level)
{
	if (level == LEVEL_C0)
		return 1;
	return cpu_linux::has_pstate_level(level);
}