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
#pragma once

#include <string>

#include "device.h"
#include "alsa.h"

/*
 * One ALSA PCM substream (/proc/asound/card<N>/pcm<D><p|c>/sub<K>).
 *
 * An open substream keeps an HDA codec (and the controller) powered even when it is idle
 * (PREPARED/SETUP), which defeats snd_hda_intel.power_save. The state is sampled at the start
 * and the end of the measurement and the row is only shown while the stream is open.
 * The row has no power model: it is listed below its codec in the Device stats tab only.
 */
class alsa_pcm: public device {
	/* "closed" or the ALSA state name (RUNNING, PREPARED, ...) plus the owning process */
	struct sample {
		std::string state = "closed";
		std::string owner;
	};

	alsa *parent;
	int card, pcm_device, substream;
	bool playback;
	std::string status_path;
	std::string pcm_name;
	std::string hw_name;
	sample start_sample, end_sample;

	static sample read_status(const std::string &path);
public:
	alsa_pcm(alsa *_parent, int _card, int _device, bool _playback, int _substream);

	void start_measurement(void) override;
	void end_measurement(void) override;

	bool is_open(void) const { return start_sample.state != "closed" || end_sample.state != "closed"; }

	std::string class_name(void) const override { return "alsa_pcm"; }
	std::string device_name(void) const override { return hw_name; }
	std::string human_name(void) override;

	double utilization(void) const override { return 0.0; }
	std::string util_units(void) const override { return ""; }
	bool power_valid(void) const override { return false; }

	bool show_in_list(void) const override { return is_open(); }
	bool show_in_overview(void) const override { return false; }
	const device *parent_device(void) const override { return parent; }

	void collect_json_fields(std::string &_js) override;
};

extern void create_all_alsa_pcm(alsa *codec);
