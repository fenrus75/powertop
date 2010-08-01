#include <string.h>
#include <stdio.h>
#include "cpu.h"

void abstract_cpu::measurement_start(void)
{
	unsigned int i;

	for (i = 0; i < states.size(); i++)
		delete states[i];
	states.resize(0);

	for (i = 0; i < this->children.size(); i++)
		if (this->children[i])
			this->children[i]->measurement_start();

}

void abstract_cpu::measurement_end(void)
{
	unsigned int i, j;
	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->measurement_end();

	for (i = 0; i < children.size(); i++)
		if (children[i])
			for (j = 0; j < children[i]->states.size(); j++) {
				struct power_state *state;
				state = children[i]->states[j];
				if (!state)
					continue;

				update_state( state->linux_name, state->human_name, state->usage_before, state->duration_before, state->before_count);
				finalize_state(state->linux_name,                   state->usage_after,  state->duration_after,  state->after_count);
			}


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

void abstract_cpu::insert_state(const char *linux_name, const char *human_name, uint64_t usage, uint64_t duration, int count)
{
	struct power_state *state;

	state = new struct power_state;

	if (!state)
		return;

	memset(state, 0, sizeof(*state));

	states.push_back(state);

	strcpy(state->linux_name, linux_name);
	strcpy(state->human_name, human_name);

	state->usage_before = usage;
	state->duration_before = duration;
	state->before_count = count;
}

void abstract_cpu::finalize_state(const char *linux_name, uint64_t usage, uint64_t duration, int count)
{
 	unsigned int i;
	struct power_state *state = NULL;


	for (i = 0; i < states.size(); i++) {
		if (strcmp(linux_name, states[i]->linux_name) == 0) {
			state = states[i];
			break;
		}
	}

	if (!state) {
		cout << "Invalid C state update " << linux_name << " \n";
		return;
	}

	state->usage_after += usage;
	state->duration_after += duration;
	state->after_count += count;
}

void abstract_cpu::update_state(const char *linux_name, const char *human_name, uint64_t usage, uint64_t duration, int count)
{
 	unsigned int i;
	struct power_state *state = NULL;

	for (i = 0; i < states.size(); i++) {
		if (strcmp(linux_name, states[i]->linux_name) == 0) {
			state = states[i];
			break;
		}
	}

	if (!state) {
		insert_state(linux_name, human_name, usage, duration, 1);
		return;
	}

	state->usage_before += usage;
	state->duration_before += duration;
	state->before_count += count;

}
