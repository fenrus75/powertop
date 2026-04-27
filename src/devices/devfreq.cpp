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
#include <sstream>
#include <format>

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

devfreq::devfreq(const string &dpath): device()
{
	dir_name = dpath;
}

uint64_t devfreq::parse_freq_time(const string &pchr_s)
{
	uint64_t ctime = 0;
	size_t pos;

	pos = pchr_s.find_last_of(" :");
	if (pos != string::npos) {
		try {
			ctime = std::stoull(pchr_s.substr(pos + 1));
		} catch (...) {}
	}

	return ctime;
}

void devfreq::process_time_stamps()
{
	unsigned int i;
	uint64_t active_time = 0;

	sample_time = (1000000.0 * (stamp_after.tv_sec - stamp_before.tv_sec))
			+ ((stamp_after.tv_usec - stamp_before.tv_usec) );

	for (i=0; i < dstates.size()-1; i++) {
		class frequency *state = dstates[i];
		state->time_after = 1000 * (state->time_after - state->time_before);
		active_time += state->time_after;
	}
	/* Compute idle time for the device */
	dstates[i]->time_after = sample_time - active_time;
}

void devfreq::add_devfreq_freq_state(uint64_t freq, uint64_t time)
{
	class frequency *state;

	state = new(std::nothrow) class frequency;
	if (!state)
		return;

	dstates.push_back(state);

	state->freq = freq;
	if (freq == 0)
		state->human_name = "Idle";
	else
		state->human_name = hz_to_human(freq);
	state->time_before = time;
}

void devfreq::update_devfreq_freq_state(uint64_t freq, uint64_t time)
{
	unsigned int i;
	class frequency *state = NULL;

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

void devfreq::parse_devfreq_trans_stat(const string &dname)
{
	std::string filename;
	std::string content;

	filename = std::format("/sys/class/devfreq/{}/trans_stat", dir_name);
	content = read_file_content(filename);

	if (content.empty())
		return;

	std::istringstream stream(content);
	std::string line;

	while (std::getline(stream, line)) {
		uint64_t freq;
		uint64_t time;
		std::string pchr;

		if (line.empty())
			continue;

		size_t pos = line.find('*');
		if (pos != std::string::npos)
			pchr = line.substr(pos + 1);
		else
			pchr = line;

		std::istringstream iss(pchr);
		if (iss >> freq) {
			if (!freq)
				continue;

			time = parse_freq_time(pchr);
			update_devfreq_freq_state(freq, time);
		}
	}
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

string devfreq::fill_freq_utilization(unsigned int idx)
{
	if (idx < dstates.size() && dstates[idx]) {
		class frequency *state = dstates[idx];
		return std::format(" {:5.1f}% ", percentage(1.0 * state->time_after / sample_time));
	}
	return "";
}

string devfreq::fill_freq_name(unsigned int idx)
{
	if (idx < dstates.size() && dstates[idx]) {
		return std::format("{:<15}", dstates[idx]->human_name);
	}
	return "";
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

static void devfreq_dev_callback(const std::string &d_name)
{
	devfreq *df = new(std::nothrow) class devfreq(d_name);
	if (df)
		all_devfreq.push_back(df);
}

void create_all_devfreq_devices(void)
{
	int num = 0;

	std::string p = "/sys/class/devfreq/";
	dir = opendir(p.c_str());
	if (dir == NULL) {
		fprintf(stderr, "Devfreq not enabled\n");
		is_enabled = false;
		return;
	}

	while(readdir(dir) != NULL)
		num++;

	if (num == 2) {
		fprintf(stderr, "Devfreq not enabled\n");
		is_enabled = false;
		closedir(dir);
		dir = NULL;
		return;
	}

	callback_str fn = &devfreq_dev_callback;
	process_directory(p, fn);
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
		wprintw(win, "\n%s\n", df->device_name().c_str());

		for(j=0; j < df->dstates.size(); j++) {
			std::string f_name = df->fill_freq_name(j);
			std::string f_util = df->fill_freq_utilization(j);
			wprintw(win, "\t%s%s\n", f_name.c_str(), f_util.c_str());
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
