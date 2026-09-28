/*
 * Copyright 2025, Intel Corporation
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
#include <fstream>
#include <format>
#include <unistd.h>
#include <memory>

extern "C" {
#include <tracefs.h>
}

#include "../lib.h"
#include "device.h"
#include "xe-gpu.h"
#include "../parameters/parameters.h"
#include "../process/powerconsumer.h"

/*
 * Scan the xe hwmon directory for all power channels (power1..powerN).
 * For each channel found, record its label, TDP cap (power*_crit), and
 * energy path (energy*_input) if the kernel exposes one.
 *
 * Returns the hwmon directory path, or empty if no "xe" hwmon is found.
 */
static std::string find_xe_hwmon_dir(void)
{
	for (const auto &entry : list_directory("/sys/class/drm")) {
		if (!entry.starts_with("card"))
			continue;
		if (entry.find('-') != std::string::npos)
			continue;

		const std::string hwmon_base =
			std::format("/sys/class/drm/{}/device/hwmon", entry);

		for (const auto &hwmon : list_directory(hwmon_base)) {
			const std::string name_path =
				std::format("{}/{}/name", hwmon_base, hwmon);
			if (read_file_content(name_path) != "xe\n")
				continue;
			return std::format("{}/{}", hwmon_base, hwmon);
		}
	}
	return {};
}

xegpu::xegpu()
	: device()
{
	index  = get_param_index("xe-gpu-operations");
	rindex = get_result_index("xe-gpu-operations");

	const std::string hwmon_dir = find_xe_hwmon_dir();
	if (hwmon_dir.empty())
		return;

	/* Scan power1..power32 for any channel that has a label file. */
	for (int n = 1; n <= 32; ++n) {
		const std::string label_path =
			std::format("{}/power{}_label", hwmon_dir, n);
		if (pt_access(label_path, R_OK) != 0)
			break;

		xe_power_channel ch;
		ch.label = read_file_content(label_path);
		/* Strip trailing newline from sysfs string. */
		if (!ch.label.empty() && ch.label.back() == '\n')
			ch.label.pop_back();

		const std::string crit_path =
			std::format("{}/power{}_crit", hwmon_dir, n);
		if (pt_access(crit_path, R_OK) == 0) {
			const uint64_t crit_uw =
				read_sysfs_uint64(crit_path, nullptr);
			ch.tdp_cap_watts = static_cast<double>(crit_uw) / 1e6;
		}

		channel_track track;
		const std::string energy_path =
			std::format("{}/energy{}_input", hwmon_dir, n);
		if (pt_access(energy_path, R_OK) == 0) {
			track.energy_path = energy_path;
			track.last_energy =
				static_cast<double>(read_sysfs_uint64(energy_path, nullptr));
			track.last_time   = pt_gettime();
		}

		power_channels.push_back(ch);
		channel_tracks.push_back(track);
	}
}

void xegpu::start_measurement(void)
{
	for (size_t i = 0; i < channel_tracks.size(); ++i) {
		auto &tr = channel_tracks[i];
		if (tr.energy_path.empty())
			continue;
		tr.last_time   = pt_gettime();
		tr.last_energy =
			(double)read_sysfs_uint64(tr.energy_path, nullptr);
	}
}

void xegpu::end_measurement(void)
{
	consumed_power = 0.0;

	for (size_t i = 0; i < channel_tracks.size(); ++i) {
		auto       &tr = channel_tracks[i];
		auto       &ch = power_channels[i];

		if (tr.energy_path.empty()) {
			ch.current_watts = -1.0;
			continue;
		}

		const struct timeval now = pt_gettime();
		const double delta =
			(now.tv_sec  - tr.last_time.tv_sec)
			+ (now.tv_usec - tr.last_time.tv_usec) / 1e6;

		if (delta < 1e-5) {
			ch.current_watts = -1.0;
			continue;
		}

		/* energy*_input is in micro-joules */
		const double energy =
			static_cast<double>(read_sysfs_uint64(tr.energy_path, nullptr));
		ch.current_watts = (energy - tr.last_energy) / 1e6 / delta;
		tr.last_energy   = energy;
		tr.last_time     = now;

		/* First channel (package) drives power_usage(). */
		if (i == 0)
			consumed_power = ch.current_watts;
	}
}

/*
 * xe_fan_device: shows a single GPU fan's RPM in the device stats tab.
 *
 * The xe hwmon exposes fan1_input .. fanN_input (in RPM).  We create one
 * device instance per fan slot that exists in sysfs.
 */
class xe_fan_device : public device {
	std::string fan_path;
	std::string fan_label;
	double      current_rpm = 0.0;

public:
	xe_fan_device(const std::string &path, const std::string &label)
		: device(), fan_path(path), fan_label(label) {}

	virtual void start_measurement(void) override {}
	virtual void end_measurement(void) override
	{
		current_rpm = static_cast<double>(
			read_sysfs_uint64(fan_path, nullptr));
	}

	virtual double      utilization(void) const override { return current_rpm; }
	virtual std::string class_name(void) const override  { return "GPU"; }
	virtual std::string device_name(void) const override { return fan_label; }
	virtual std::string human_name(void) override        { return fan_label; }
	virtual double      power_usage([[maybe_unused]] struct result_bundle *r,
				        [[maybe_unused]] struct parameter_bundle *b) override
			{ return 0.0; }
	virtual bool        show_in_list(void) const override { return true; }
	virtual std::string util_units(void) const override   { return _(" RPM"); }

	void collect_json_fields(std::string &_js) override
	{
		device::collect_json_fields(_js);
		JSON_FIELD(fan_path);
		JSON_FIELD(fan_label);
		JSON_FIELD(current_rpm);
	}
};

/*
 * Scan the xe hwmon for fan*_input files and register one xe_fan_device per
 * fan slot found.  Slots reporting 0 RPM are still registered (the fan may
 * simply be stopped or at low load).
 */
static void create_xe_fans(void)
{
	for (const auto &entry : list_directory("/sys/class/drm")) {
		if (!entry.starts_with("card"))
			continue;
		if (entry.find('-') != std::string::npos)
			continue;

		const std::string hwmon_base =
			std::format("/sys/class/drm/{}/device/hwmon", entry);

		for (const auto &hwmon : list_directory(hwmon_base)) {
			const std::string name_path =
				std::format("{}/{}/name", hwmon_base, hwmon);
			if (read_file_content(name_path) != "xe\n")
				continue;

			const std::string hwmon_path =
				std::format("{}/{}", hwmon_base, hwmon);

			for (int n = 1; ; ++n) {
				const std::string fan_path =
					std::format("{}/fan{}_input",
						    hwmon_path, n);
				if (pt_access(fan_path, R_OK) != 0)
					break;
				const std::string label =
					pt_format(_("Xe GPU Fan {}"), n);
				all_devices.push_back(
					std::make_unique<xe_fan_device>(fan_path, label));
			}
		}
	}
}

double xegpu::utilization(void) const
{
	return get_result_value(rindex);
}

double xegpu::power_usage(struct result_bundle *result,
			   struct parameter_bundle *bundle)
{
	if (!channel_tracks.empty() && !channel_tracks[0].energy_path.empty())
		return consumed_power;

	const double factor = get_parameter_value(index, bundle);
	const double util   = get_result_value(rindex, result);
	return util * factor / 100.0;
}

void xegpu::collect_json_fields(std::string &_js)
{
	device::collect_json_fields(_js);
	JSON_FIELD(index);
	JSON_FIELD(rindex);
	JSON_FIELD(consumed_power);
	for (size_t i = 0; i < power_channels.size(); ++i) {
		const auto &ch = power_channels[i];
		JSON_KV(std::format("channel{}_label", i),   ch.label);
		JSON_KV(std::format("channel{}_watts", i),   ch.current_watts);
		JSON_KV(std::format("channel{}_tdp", i),     ch.tdp_cap_watts);
	}
}

void create_xe_gpu(void)
{
	if (!tracefs_event_file_exists(nullptr, "xe",
				       "xe_sched_job_exec", "format"))
		return;

	register_parameter("xe-gpu-operations");

	all_devices.push_back(std::make_unique<xegpu>());

	create_xe_fans();
}
