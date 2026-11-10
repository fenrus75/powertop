/*
 * Copyright 2010, Intel Corporation
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
 *	Arjan van de Ven <arjan@linux.intel.com>
 */
#include <iostream>
#include <fstream>
#include <sstream>

#include <cstdio>
#include <sys/types.h>
#include <unistd.h>

#include "device.h"
#include "alsa.h"
#include "../parameters/parameters.h"

#include "../devlist.h"

#include <unistd.h>
#include <format>

alsa::alsa(const std::string &_name, const std::string &path): device()
{
	std::string model;
	std::string vendor;
	end_active = 0;
	start_active = 0;
	end_inactive = 0;
	start_inactive = 0;
	sysfs_path = path;

	name = std::format("alsa:{}", _name);
	humanname = std::format("alsa:{}", _name);
	rindex = get_result_index(name);

	if (_name.starts_with("hwC")) {
		const size_t d = _name.find('D', 3);
		if (d != std::string::npos && d > 3) {
			try {
				card = std::stoi(_name.substr(3, d - 3));
			} catch (...) {}
		}
	}

	model = read_sysfs_string(std::format("{}/modelname", path));
	vendor = read_sysfs_string(std::format("{}/vendor_name", path));

	if (!model.empty() && !vendor.empty())
		humanname = pt_format(_("Audio codec {}: {} ({})"), name, model, vendor);
	else if (!model.empty())
		humanname = pt_format(_("Audio codec {}: {}"), _name, model);
	else if (!vendor.empty())
		humanname = pt_format(_("Audio codec {}: {}"), _name, vendor);
}

/*
 * Each substream has /proc/asound/card<N>/pcm<D><p|c>/sub<K>/status whose first
 * line is either "closed" or "state: <NAME>", followed by "owner_pid : <pid>".
 * The file only exists with CONFIG_SND_VERBOSE_PROCFS; without it nothing is reported.
 * Returns pcm name (with ".<K>" for K > 0) -> "STATE|owner" for open substreams.
 */
std::map<std::string, std::string> alsa::read_pcm_streams(void)
{
	std::map<std::string, std::string> streams;

	if (card < 0)
		return streams;

	const std::string base = std::format("/proc/asound/card{}", card);

	for (const auto &pcm : list_directory(base)) {
		if (!pcm.starts_with("pcm"))
			continue;

		for (const auto &sub : list_directory(std::format("{}/{}", base, pcm))) {
			if (!sub.starts_with("sub"))
				continue;

			const std::string status = read_file_content(std::format("{}/{}/{}/status", base, pcm, sub));
			std::string state;
			std::string owner;
			bool open = false;

			std::istringstream ss(status);
			std::string line;
			while (std::getline(ss, line)) {
				if (line.starts_with("state:")) {
					state = line.substr(6);
					state.erase(0, state.find_first_not_of(" \t"));
					open = !state.empty();
				} else if (line.starts_with("owner_pid")) {
					const size_t colon = line.find(':');
					if (colon == std::string::npos)
						continue;
					try {
						const int pid = std::stoi(line.substr(colon + 1));
						if (pid > 0)
							owner = read_sysfs_string(std::format("/proc/{}/comm", pid));
					} catch (...) {}
				}
			}

			if (!open)
				continue;

			std::string key = pcm;
			if (sub != "sub0")
				key += "." + sub.substr(3);
			streams[key] = state + "|" + owner;
		}
	}

	return streams;
}

/* e.g. "pcm0p RUNNING: pipewire, pcm0c PREPARED (idle): pipewire" */
std::string alsa::format_streams(const std::map<std::string, std::string> &start,
				 const std::map<std::string, std::string> &end)
{
	std::map<std::string, std::string> all = start;
	for (const auto &[key, value] : end)
		all[key] = value;

	std::string result;
	for (const auto &[key, value] : all) {
		const size_t bar = value.find('|');
		const std::string state = value.substr(0, bar);
		const std::string owner = (bar == std::string::npos) ? "" : value.substr(bar + 1);
		const bool idle = state == "OPEN" || state == "SETUP" || state == "PREPARED";

		if (!result.empty())
			result += ", ";
		result += std::format("{} {}{}", key, state, idle ? " (idle)" : "");
		if (!owner.empty())
			result += ": " + owner;
	}
	return result;
}

void alsa::start_measurement(void)
{
	std::string content;

	start_streams = read_pcm_streams();

	content = read_sysfs_string(std::format("{}/power_off_acct", sysfs_path));
	if (!content.empty()) {
		try {
			start_inactive = std::stoull(content);
		} catch (...) {}
	}

	content = read_sysfs_string(std::format("{}/power_on_acct", sysfs_path));
	if (!content.empty()) {
		try {
			start_active = std::stoull(content);
		} catch (...) {}
	}
}

void alsa::end_measurement(void)
{
	std::string content;
	double p;

	end_streams = read_pcm_streams();
	stream_summary = format_streams(start_streams, end_streams);

	content = read_sysfs_string(std::format("{}/power_off_acct", sysfs_path));
	if (!content.empty()) {
		try {
			end_inactive = std::stoull(content);
		} catch (...) {}
	}

	content = read_sysfs_string(std::format("{}/power_on_acct", sysfs_path));
	if (!content.empty()) {
		try {
			end_active = std::stoull(content);
		} catch (...) {}
	}

	const double active_delta = (end_active >= start_active) ? (double)(end_active - start_active) : 0.0;
	const double total_delta = (end_active + end_inactive >= start_active + start_inactive) ?
		(double)(end_active + end_inactive - start_active - start_inactive) : 0.0;

	if (total_delta < 0.001)
		p = 0.0;
	else
		p = active_delta / total_delta * 100.0;

	report_utilization(name, p);
}


double alsa::utilization(void) const
{
	double p;

	const double active_delta = (end_active >= start_active) ? (double)(end_active - start_active) : 0.0;
	const double total_delta = (end_active + end_inactive >= start_active + start_inactive) ?
		(double)(end_active + end_inactive - start_active - start_inactive) : 0.0;

	if (total_delta < 0.001)
		p = 0.0;
	else
		p = active_delta / total_delta * 100.0;

	return p;
}

static void create_all_alsa_callback(const std::string &d_name)
{
	if (!d_name.starts_with("hwC"))
		return;

	if (pt_access(std::format("/sys/class/sound/{}/power_on_acct", d_name), R_OK) != 0)
		return;

	all_devices.push_back(std::make_unique<alsa>(d_name, std::format("/sys/class/sound/{}", d_name)));
	register_parameter("alsa-codec-power", 0.5);
}

void create_all_alsa(void)
{
	process_directory("/sys/class/sound/", create_all_alsa_callback);
}

double alsa::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;
	static int index = -1;

	power = 0;
	if (index < 0)
		index = get_param_index("alsa-codec-power");

	factor = get_parameter_value(index, bundle);

	util = get_result_value(rindex, result);

	power += util * factor / 100.0;

	return power;
}

void alsa::register_power_with_devlist(struct result_bundle *results, struct parameter_bundle *bundle)
{
	if (name.length() > 7)
		register_devpower(name.substr(7), power_usage(results, bundle), this);
	else
		register_devpower(name, power_usage(results, bundle), this);
}

std::string alsa::human_name(void)
{
	std::string result = humanname;

	if (!stream_summary.empty())
		result += std::format(" [{}]", stream_summary);
	if (!guilty.empty())
		result += std::format(" ({})", guilty);

	return result;
}

void alsa::collect_json_fields(std::string &_js)
{
    device::collect_json_fields(_js);
    JSON_FIELD(start_active);
    JSON_FIELD(end_active);
    JSON_FIELD(start_inactive);
    JSON_FIELD(end_inactive);
    JSON_FIELD(sysfs_path);
    JSON_FIELD(name);
    JSON_FIELD(humanname);
    JSON_FIELD(rindex);
    JSON_FIELD(stream_summary);
}
