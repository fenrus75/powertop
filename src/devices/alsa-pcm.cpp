/*
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
 */
#include <cstdio>
#include <format>
#include <sstream>

#include "alsa-pcm.h"
#include "../devlist.h"

alsa_pcm::alsa_pcm(alsa *_parent, int _card, int _device, bool _playback, int _substream):
	device(), parent(_parent), card(_card), pcm_device(_device), substream(_substream),
	playback(_playback)
{
	const std::string dir = std::format("/proc/asound/card{}/pcm{}{}", card, pcm_device, playback ? 'p' : 'c');

	status_path = std::format("{}/sub{}/status", dir, substream);
	hw_name = std::format("hw:{},{}", card, pcm_device);
	if (substream > 0)
		hw_name += std::format(",{}", substream);

	/* "name: ALC257 Analog" in the PCM info file */
	std::istringstream info(read_file_content(dir + "/info"));
	std::string line;
	while (std::getline(info, line)) {
		if (line.starts_with("name:")) {
			pcm_name = line.substr(5);
			pcm_name.erase(0, pcm_name.find_first_not_of(" \t"));
			break;
		}
	}
}

/*
 * The status file starts with either "closed" or "state: <NAME>", followed by "owner_pid : <pid>".
 * It only exists with CONFIG_SND_VERBOSE_PROCFS; without it the stream is reported as closed.
 */
alsa_pcm::sample alsa_pcm::read_status(const std::string &path)
{
	sample result;
	std::istringstream ss(read_file_content(path));
	std::string line;
	std::string state;
	int pid = 0;

	while (std::getline(ss, line)) {
		if (line.starts_with("state:")) {
			state = line.substr(6);
			state.erase(0, state.find_first_not_of(" \t"));
		} else if (line.starts_with("owner_pid")) {
			const size_t colon = line.find(':');
			if (colon == std::string::npos)
				continue;
			try {
				pid = std::stoi(line.substr(colon + 1));
			} catch (...) {}
		}
	}

	if (state.empty())
		return result;

	result.state = state;
	if (pid > 0) {
		const std::string comm = read_sysfs_string(std::format("/proc/{}/comm", pid));
		result.owner = std::format("{} [{}]", comm.empty() ? "?" : comm, pid);
	}
	return result;
}

void alsa_pcm::start_measurement(void)
{
	device::start_measurement();
	start_sample = read_status(status_path);
}

void alsa_pcm::end_measurement(void)
{
	device::end_measurement();
	end_sample = read_status(status_path);
}

std::string alsa_pcm::human_name(void)
{
	const bool idle = [](const std::string &s) { return s == "OPEN" || s == "SETUP" || s == "PREPARED"; }(end_sample.state);
	std::string state = end_sample.state;
	const sample &who = end_sample.owner.empty() ? start_sample : end_sample;

	if (start_sample.state != end_sample.state)
		state = std::format("{} -> {}", start_sample.state, end_sample.state);
	else if (idle)
		state += " (idle)";

	std::string result = std::format("PCM {} {}", hw_name, playback ? "playback" : "capture");
	if (!pcm_name.empty())
		result += std::format(" ({})", pcm_name);
	result += ": " + state;
	if (!who.owner.empty())
		result += ", " + who.owner;
	return result;
}

void alsa_pcm::collect_json_fields(std::string &_js)
{
	device::collect_json_fields(_js);
	JSON_FIELD(hw_name);
	JSON_FIELD(pcm_name);
	JSON_FIELD(playback);
	JSON_KV("start_state", start_sample.state);
	JSON_KV("end_state", end_sample.state);
	JSON_KV("start_owner", start_sample.owner);
	JSON_KV("end_owner", end_sample.owner);
}

/* Creates one alsa_pcm per existing substream of the codec's card; they are shown only while open */
void create_all_alsa_pcm(alsa *codec)
{
	const int card = codec->card_number();
	const std::string base = std::format("/proc/asound/card{}", card);

	for (const auto &pcm : list_directory(base)) {
		int pcm_device = -1;
		char dir = 0;

		if (std::sscanf(pcm.c_str(), "pcm%d%c", &pcm_device, &dir) != 2 || (dir != 'p' && dir != 'c'))
			continue;
		if (pcm.size() != std::format("pcm{}{}", pcm_device, dir).size())
			continue;

		for (const auto &sub : list_directory(std::format("{}/{}", base, pcm))) {
			int substream = -1;

			if (std::sscanf(sub.c_str(), "sub%d", &substream) != 1 || substream < 0)
				continue;

			auto stream = std::make_unique<alsa_pcm>(codec, card, pcm_device, dir == 'p', substream);
			codec->add_stream(stream.get());
			all_devices.push_back(std::move(stream));
		}
	}
}
