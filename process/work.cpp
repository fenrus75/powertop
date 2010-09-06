#include <map>
#include <utility>

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "work.h"
#include "../lib.h"

using namespace std;


work::work(unsigned long address)
{
	strncpy(handler, kernel_function(address), 31);
	wake_ups = 0;
	disk_hits = 0;
	accumulated_runtime = 0;
	child_runtime = 0;
	waker = NULL;
	raw_count = 0;
}


static map<unsigned long, class work *> all_work;
static map<unsigned long, uint64_t> running_since;

void work::fire(uint64_t time, uint64_t work_struct)
{
	running_since[work_struct] = time;
}

uint64_t work::done(uint64_t time, uint64_t work_struct)
{
	int64_t delta;

	if (running_since[work_struct] > time)
		return 0;

	delta = time - running_since[work_struct];
	if (delta < 0)
		printf("GOT HERE %llin", delta);
	accumulated_runtime += delta;

	raw_count++;

	return delta;
}



static void add_work(const pair<unsigned long, class work*>& elem)
{
	all_power.push_back(elem.second);
}

void all_work_to_all_power(void)
{
	for_each(all_work.begin(), all_work.end(), add_work);

}


const char * work::description(void)
{
	if (child_runtime > accumulated_runtime)
		child_runtime = 0;

	sprintf(desc, "Work  %24s      time  %5.2fms    wakeups %3i  (total: %i)",
			handler,  (accumulated_runtime - child_runtime) / 1000000.0, wake_ups, raw_count);
	return desc;
}


class work * find_create_work(uint64_t func)
{
	class work * work;
	if (all_work[func])
		return all_work[func];

	work = new class work(func);
	all_work[func] = work;
	return work;
	
}