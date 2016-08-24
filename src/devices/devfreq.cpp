/*
 * Copyright 2012, Linaro
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Rajagopal Venkat <rajagopal.venkat@linaro.org>
 */

#include <iostream>
#include <fstream>

#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "device.h"
#include "devfreq.h"
#include "../display.h"
#include "../cpu/cpu.h"
#include "../report/report.h"
#include "../report/report-maker.h"

static bool is_enabled = true;
static DIR *dir = NULL;

static vector<class devfreq *> all_devfreq;

devfreq::devfreq(const char* dpath): device()
{
	pt_strcpy(dir_name, dpath);
}

uint64_t devfreq::parse_freq_time(char* pchr)
{
	char *cptr, *pptr = pchr;
	uint64_t ctime;

	cptr = strtok(pchr, " :");
	while (cptr != NULL) {
		cptr = strtok(NULL, " :");
		if (cptr )
			pptr = cptr;
	}

	ctime = strtoull(pptr, NULL, 10);
	return ctime;
}

void devfreq::process_time_stamps()
{
	unsigned int i;
	uint64_t active_time = 0;

	sample_time = (1000000.0 * (stamp_after.tv_sec - stamp_before.tv_sec))
			+ ((stamp_after.tv_usec - stamp_before.tv_usec) );

	for (i=0; i < dstates.size()-1; i++) {
		struct frequency *state = dstates[i];
		state->time_after = 1000 * (state->time_after - state->time_before);
		active_time += state->time_after;
	}
	/* Compute idle time for the device */
	dstates[i]->time_after = sample_time - active_time;
}

void devfreq::add_devfreq_freq_state(uint64_t freq, uint64_t time)
{
	struct frequency *state;

	state = new(std::nothrow) struct frequency;
	if (!state)
		return;

	memset(state, 0, sizeof(*state));
	dstates.push_back(state);

	state->freq = freq;
	if (freq == 0)
		strcpy(state->human_name, "Idle");
	else
		hz_to_human(freq, state->human_name);
	state->time_before = time;
}

void devfreq::update_devfreq_freq_state(uint64_t freq, uint64_t time)
{
	unsigned int i;
	struct frequency *state = NULL;

	for(i=0; i < dstates.size(); i++) {
		if (freq == dstates[i]->freq)
			state = dstates[i];
	}

	if (state == NULL) {
		add_devfreq_freq_state(freq, time);
		return;
	}

	state->time_after = time;
}

void devfreq::parse_devfreq_trans_stat(char *dname)
{
	ifstream file;
	char filename[256];

	snprintf(filename, sizeof(filename), "/sys/class/devfreq/%s/trans_stat", dir_name);
	file.open(filename);

	if (!file)
		return;

	char line[1024];
	char *c;

	while (file) {
		uint64_t freq;
		uint64_t time;
		char *pchr;

		memset(line, 0, sizeof(line));
		file.getline(line, sizeof(line));

		pchr = strchr(line, '*');
		pchr = (pchr != NULL) ? pchr+1 : line;

		freq = strtoull(pchr, &c, 10);
		if (!freq)
			continue;

		time = parse_freq_time(pchr);
		update_devfreq_freq_state(freq, time);
	}
	file.close();
}

void devfreq::start_measurement(void)
{
	unsigned int i;

	for (i=0; i < dstates.size(); i++)
		delete dstates[i];
	dstates.resize(0);
	sample_time = 0;

	gettimeofday(&stamp_before, NULL);
	parse_devfreq_trans_stat(dir_name);
	/* add device idle state */
	update_devfreq_freq_state(0, 0);
}

void devfreq::end_measurement(void)
{
	parse_devfreq_trans_stat(dir_name);
	gettimeofday(&stamp_after, NULL);
	process_time_stamps();
}

double devfreq::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	return 0;
}

double devfreq::utilization(void)
{
	return 0;
}

void devfreq::fill_freq_utilization(unsigned int idx, char *buf)
{
	buf[0] = 0;

	if (idx < dstates.size() && dstates[idx]) {
		struct frequency *state = dstates[idx];
		sprintf(buf, " %5.1f%% ", percentage(1.0 * state->time_after / sample_time));
	}
}

void devfreq::fill_freq_name(unsigned int idx, char *buf)
{
	buf[0] = 0;

	if (idx < dstates.size() && dstates[idx]) {
		sprintf(buf, "%-15s", dstates[idx]->human_name);
	}
}

void start_devfreq_measurement(void)
{
	unsigned int i;

	for (i=0; i<all_devfreq.size(); i++)
		all_devfreq[i]->start_measurement();
}

void end_devfreq_measurement(void)
{
	unsigned int i;

	for (i=0; i<all_devfreq.size(); i++)
		all_devfreq[i]->end_measurement();
}

static void devfreq_dev_callback(const char *d_name)
{
	devfreq *df = new(std::nothrow) class devfreq(d_name);
	if (df)
		all_devfreq.push_back(df);
}

void create_all_devfreq_devices(void)
{
	struct dirent *entry;
	int num = 0;

	std::string p = "/sys/class/devfreq/";
	dir = opendir(p.c_str());
	if (dir == NULL) {
		fprintf(stderr, "Devfreq not enabled\n");
		is_enabled = false;
		return;
	}

	while((entry = readdir(dir)) != NULL)
		num++;

	if (num == 2) {
		fprintf(stderr, "Devfreq not enabled\n");
		is_enabled = false;
		closedir(dir);
		dir = NULL;
		return;
	}

	callback fn = &devfreq_dev_callback;
	process_directory(p.c_str(), fn);
}

void initialize_devfreq(void)
{
	if (is_enabled)
		create_tab("Device Freq stats", _("Device Freq stats"));
}

void display_devfreq_devices(void)
{
	unsigned int i, j;
	WINDOW *win;
	char fline[1024];
	char buf[128];

	win = get_ncurses_win("Device Freq stats");
        if (!win)
                return;

        wclear(win);
        wmove(win, 2,0);

	if (!is_enabled) {
		wprintw(win, _(" Devfreq is not enabled"));
		return;
	}

	if (!all_devfreq.size()) {
		wprintw(win, _(" No devfreq devices available"));
		return;
	}

	for (i=0; i<all_devfreq.size(); i++) {

		class devfreq *df = all_devfreq[i];
		wprintw(win, "\n%s\n", df->device_name());

		for(j=0; j < df->dstates.size(); j++) {
			memset(fline, 0, sizeof(fline));
			strcpy(fline, "\t");
			df->fill_freq_name(j, buf);
			strcat(fline, buf);
			df->fill_freq_utilization(j, buf);
			strcat(fline, buf);
			strcat(fline, "\n");
			wprintw(win, fline);
		}
		wprintw(win, "\n");
	}
}

void report_devfreq_devices(void)
{
	if (!is_enabled) {
		return;
	}

/* todo: adapt to new report format */

}

void clear_all_devfreq()
{
	unsigned int i, j;

	for (i=0; i < all_devfreq.size(); i++) {
		class devfreq *df = all_devfreq[i];

		for(j=0; j < df->dstates.size(); j++)
			delete df->dstates[j];

		delete df;
	}
	all_devfreq.clear();
	/* close /sys/class/devfreq */
	if (dir != NULL) {
		closedir(dir);
		dir = NULL;
	}
}
