/* rapl_interface.h: rapl interface for power top
 *
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 *
 * Author Name <Srinivas.Pandruvada@linux.intel.com>
 *
 */

#ifndef RAPL_INTERFACE_H
#define RAPL_INTERFACE_H

class c_rapl_interface
{
private:
	static const int def_sampling_interval = 1; //In seconds
	bool powercap_sysfs_present;
	string powercap_core_path;
	string powercap_uncore_path;
	string powercap_dram_path;

	unsigned char rapl_domains;
	int first_cpu;

	double power_units;
	double energy_status_units;
	double time_units;

	int read_msr(int cpu, unsigned int idx, uint64_t *val);
	int write_msr(int cpu, unsigned int idx, uint64_t val);

protected:
	int measurment_interval;
	double last_pkg_energy_status;
	double last_dram_energy_status;
	double last_pp0_energy_status;
	double last_pp1_energy_status;

public:
	c_rapl_interface(const char *dev_name = "package-0", int cpu = 0);

	int get_rapl_power_unit(uint64_t *value);
	double get_power_unit();
	double get_energy_status_unit();
	double get_time_unit();

	int get_pkg_energy_status(double *status);
	int get_pkg_power_info(double *thermal_spec_power,
			double *max_power, double *min_power, double *max_time_window);
	int get_pkg_power_limit(uint64_t *value);
	int set_pkg_power_limit(uint64_t value);

	int get_dram_energy_status(double *status);
	int get_dram_power_info(double *thermal_spec_power,
			double *max_power, double *min_power, double *max_time_window);
	int get_dram_power_limit(uint64_t *value);
	int set_dram_power_limit(uint64_t value);

	int get_pp0_energy_status(double *status);
	int get_pp0_power_limit(uint64_t *value);
	int set_pp0_power_limit(uint64_t value);
	int get_pp0_power_policy(unsigned int *pp0_power_policy);

	int get_pp1_energy_status(double *status);
	int get_pp1_power_limit(uint64_t *value);
	int set_pp1_power_limit(uint64_t value);
	int get_pp1_power_policy(unsigned int *pp1_power_policy);

	bool pkg_domain_present();
	bool dram_domain_present();
	bool pp0_domain_present();
	bool pp1_domain_present();

	void rapl_measure_energy();
};

#endif
