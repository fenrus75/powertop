/* rapl_interface.cpp: rapl interface for power top implementation
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include "lib.h"
#include "rapl_interface.h"

#ifdef DEBUG
#define RAPL_DBG_PRINT printf
#define RAPL_ERROR_PRINT printf
#else
#define RAPL_DBG_PRINT(...)	((void) 0)
#define RAPL_ERROR_PRINT(...) ((void) 0)
#endif
#define RAPL_INFO_PRINT printf

#define MAX_TEMP_STR_SIZE	20

// RAPL interface
#define MSR_RAPL_POWER_UNIT	0x606
#define MSR_PKG_POWER_LIMIT	0x610

#define MSR_PKG_ENERY_STATUS	0x611
#define MSR_PKG_POWER_INFO	0x614
#define MSR_PKG_PERF_STATUS	0x613

#define MSR_DRAM_POWER_LIMIT	0x618
#define MSR_DRAM_ENERY_STATUS	0x619
#define MSR_DRAM_PERF_STATUS	0x61B
#define MSR_DRAM_POWER_INFO	0x61c

#define MSR_PP0_POWER_LIMIT	0x638
#define MSR_PP0_ENERY_STATUS	0x639
#define MSR_PP0_POLICY		0x63A
#define MSR_PP0_PERF_STATUS	0x63B

#define MSR_PP1_POWER_LIMIT	0x640
#define MSR_PP1_ENERY_STATUS	0x641
#define MSR_PP1_POLICY		0x642

#define PKG_DOMAIN_PRESENT	0x01
#define DRAM_DOMAIN_PRESENT	0x02
#define PP0_DOMAIN_PRESENT	0x04
#define PP1_DOMAIN_PRESENT	0x08

c_rapl_interface::c_rapl_interface(int cpu) :
	measurment_interval(def_sampling_interval),
	first_cpu(cpu),
	last_pkg_energy_status(0.0),
	last_dram_energy_status(0.0),
	last_pp0_energy_status(0.0),
	last_pp1_energy_status(0.0)
{
	uint64_t value;
	int ret;

	RAPL_INFO_PRINT("RAPL device for cpu %d\n", cpu);

	rapl_domains = 0;
	// presence of each domain
	// Check presence of PKG domain
	ret = read_msr(first_cpu, MSR_PKG_ENERY_STATUS, &value);
	if (ret > 0) {
		rapl_domains |= PKG_DOMAIN_PRESENT;
		RAPL_DBG_PRINT("Domain : PKG present\n");
	} else {
		RAPL_DBG_PRINT("Domain : PKG Not present\n");
	}

	// Check presence of DRAM domain
	ret = read_msr(first_cpu, MSR_DRAM_ENERY_STATUS, &value);
	if (ret > 0) {
		rapl_domains |= DRAM_DOMAIN_PRESENT;
		RAPL_DBG_PRINT("Domain : DRAM present\n");
	} else {
		RAPL_DBG_PRINT("Domain : DRAM Not present\n");
	}

	// Check presence of PP0 domain
	ret = read_msr(first_cpu, MSR_PP0_ENERY_STATUS, &value);
	if (ret > 0) {
		rapl_domains |= PP0_DOMAIN_PRESENT;
		RAPL_DBG_PRINT("Domain : PP0 present\n");
	} else {
		RAPL_DBG_PRINT("Domain : PP0 Not present\n");
	}

	// Check presence of PP1 domain
	ret = read_msr(first_cpu, MSR_PP1_ENERY_STATUS, &value);
	if (ret > 0) {
		rapl_domains |= PP1_DOMAIN_PRESENT;
		RAPL_DBG_PRINT("Domain : PP1 present\n");
	} else {
		RAPL_DBG_PRINT("Domain : PP1 Not present\n");
	}

	power_units = get_power_unit();
	energy_status_units = get_energy_status_unit();
	time_units = get_time_unit();

	RAPL_DBG_PRINT("RAPL Domain mask: %x\n", rapl_domains);
}

bool c_rapl_interface::pkg_domain_present()
{
	if ((rapl_domains & PKG_DOMAIN_PRESENT)) {
		return true;
	}

	return false;
}

bool c_rapl_interface::dram_domain_present()
{
	if ((rapl_domains & DRAM_DOMAIN_PRESENT)) {
		return true;
	}

	return false;
}

bool c_rapl_interface::pp0_domain_present()
{
	if ((rapl_domains & PP0_DOMAIN_PRESENT)) {
		return true;
	}

	return false;
}

bool c_rapl_interface::pp1_domain_present()
{
	if ((rapl_domains & PP1_DOMAIN_PRESENT)) {
		return true;
	}

	return false;
}

int c_rapl_interface::read_msr(int cpu, unsigned int idx, uint64_t *val)
{
	return ::read_msr(cpu, idx, val);
}

int c_rapl_interface::write_msr(int cpu, unsigned int idx, uint64_t val)
{
	return ::write_msr(cpu, idx, val);
}

int c_rapl_interface::get_rapl_power_unit(uint64_t *value)
{
	int ret;

	ret = read_msr(first_cpu, MSR_RAPL_POWER_UNIT, value);

	return ret;
}

double c_rapl_interface::get_power_unit()
{
	int ret;
	uint64_t value;

	ret = get_rapl_power_unit(&value);
	if(ret < 0)
	{
		return ret;
	}

	return (double) 1/pow((double)2, (double)(value & 0xf));
}

double c_rapl_interface::get_energy_status_unit()
{
	int ret;
	uint64_t value;

	ret = get_rapl_power_unit(&value);
	if(ret < 0)
	{
		return ret;
	}

	return (double)1/ pow((double)2, (double)((value & 0x1f00) >> 8));
}

double c_rapl_interface::get_time_unit()
{
	int ret;
	uint64_t value;

	ret = get_rapl_power_unit(&value);
	if(ret < 0)
	{
		return ret;
	}

	return (double)1 / pow((double)2, (double)((value & 0xf0000) >> 16));
}

int c_rapl_interface::get_pkg_energy_status(double *status)
{
	int ret;
	uint64_t value;

	if (!pkg_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PKG_ENERY_STATUS, &value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_pkg_energy_status failed\n");
		return ret;
	}

	*status = (double) (value & 0xffffffff) * get_energy_status_unit();

	return ret;
}

int c_rapl_interface::get_pkg_power_info(double *thermal_spec_power,
			double *max_power, double *min_power, double *max_time_window)
{
	int ret;
	uint64_t value;

	if (!pkg_domain_present()) {
		return -1;
	}
	ret = read_msr(first_cpu, MSR_PKG_POWER_INFO, &value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_pkg_power_info failed\n");
		return ret;
	}
	*thermal_spec_power =  (value & 0x7FFF) * power_units;
	*min_power =  ((value & 0x7FFF0000) >> 16) * power_units;
	*max_power =  ((value & 0x7FFF00000000) >> 32) * power_units;
	*max_time_window = ((value & 0x3f000000000000)>>48) * time_units;

	return ret;
}

int c_rapl_interface::get_pkg_power_limit(uint64_t *value)
{
	int ret;

	if (!pkg_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PKG_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_pkg_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::set_pkg_power_limit(uint64_t value)
{
	int ret;

	if (!pkg_domain_present()) {
		return -1;
	}

	ret = write_msr(first_cpu, MSR_PKG_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("set_pkg_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::get_dram_energy_status(double *status)
{
	int ret;
	uint64_t value;

	if (!dram_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_DRAM_ENERY_STATUS, &value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_dram_energy_status failed\n");
		return ret;
	}

	*status = (double) (value & 0xffffffff) * get_energy_status_unit();

	return ret;
}

int c_rapl_interface::get_dram_power_info(double *thermal_spec_power,
			double *max_power, double *min_power, double *max_time_window)
{
	int ret;
	uint64_t value;

	if (!dram_domain_present()) {
		return -1;
	}
	ret = read_msr(first_cpu, MSR_DRAM_POWER_INFO, &value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_dram_power_info failed\n");
		return ret;
	}

	*thermal_spec_power =  (value & 0x7FFF) * power_units;
	*min_power =  ((value & 0x7FFF0000) >> 16) * power_units;
	*max_power =  ((value & 0x7FFF00000000) >> 32) * power_units;
	*max_time_window = ((value & 0x3f000000000000)>>48) * time_units;

	return ret;
}

int c_rapl_interface::get_dram_power_limit(uint64_t *value)
{
	int ret;

	if (!dram_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_DRAM_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_dram_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::set_dram_power_limit(uint64_t value)
{
	int ret;

	if (!dram_domain_present()) {
		return -1;
	}

	ret = write_msr(first_cpu, MSR_DRAM_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("set_dram_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::get_pp0_energy_status(double *status)
{
	int ret;
	uint64_t value;

	if (!pp0_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PP0_ENERY_STATUS, &value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_pp0_energy_status failed\n");
		return ret;
	}

	*status = (double) (value & 0xffffffff) * get_energy_status_unit();

	return ret;
}

int c_rapl_interface::get_pp0_power_limit(uint64_t *value)
{
	int ret;

	if (!pp0_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PP0_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_pp0_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::set_pp0_power_limit(uint64_t value)
{
	int ret;

	if (!pp0_domain_present()) {
		return -1;
	}

	ret = write_msr(first_cpu, MSR_PP0_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("set_pp0_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::get_pp0_power_policy(unsigned int *pp0_power_policy)
{
	int ret;
	uint64_t value;

	if (!pp0_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PP0_POLICY, &value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_pp0_power_policy failed\n");
		return ret;
	}

	*pp0_power_policy =  value & 0x0f;

	return ret;
}

int c_rapl_interface::get_pp1_energy_status(double *status)
{
	int ret;
	uint64_t value;

	if (!pp1_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PP1_ENERY_STATUS, &value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_pp1_energy_status failed\n");
		return ret;
	}

	*status = (double) (value & 0xffffffff) * get_energy_status_unit();

	return ret;
}

int c_rapl_interface::get_pp1_power_limit(uint64_t *value)
{
	int ret;

	if (!pp1_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PP1_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_pp1_power_info failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::set_pp1_power_limit(uint64_t value)
{
	int ret;

	if (!pp1_domain_present()) {
		return -1;
	}

	ret = write_msr(first_cpu, MSR_PP1_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("set_pp1_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::get_pp1_power_policy(unsigned int *pp1_power_policy)
{
	int ret;
	uint64_t value;

	if (!pp1_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PP1_POLICY, &value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_pp1_power_policy failed\n");
		return ret;
	}

	*pp1_power_policy =  value & 0x0f;

	return ret;
}

void c_rapl_interface::rapl_measure_energy()
{
#ifdef RAPL_TEST_MODE
	int ret;
	double energy_status;
	double thermal_spec_power;
	double max_power;
	double min_power;
	double max_time_window;
	double pkg_watts = 0;
	double dram_watts = 0;
	double pp0_watts = 0;
	double pp1_watts = 0;
	double pkg_joules = 0;
	double dram_joules = 0;
	double pp0_joules = 0;
	double pp1_joules = 0;

	ret = get_pkg_power_info(&thermal_spec_power, &max_power, &min_power, &max_time_window);
	RAPL_DBG_PRINT("Pkg Power Info: Thermal spec %f watts, max %f watts, min %f watts, max time window %f seconds\n", thermal_spec_power, max_power, min_power, max_time_window);
	ret = get_dram_power_info(&thermal_spec_power, &max_power, &min_power, &max_time_window);
	RAPL_DBG_PRINT("DRAM Power Info: Thermal spec %f watts, max %f watts, min %f watts, max time window %f seconds\n", thermal_spec_power, max_power, min_power, max_time_window);

	for (;;) {
		if (pkg_domain_present()) {
			ret = get_pkg_energy_status(&energy_status);
			if (last_pkg_energy_status == 0)
				last_pkg_energy_status = energy_status;
			if (ret > 0) {
				pkg_joules = energy_status;
				pkg_watts = (energy_status-last_pkg_energy_status)/measurment_interval;
			}
			last_pkg_energy_status = energy_status;
		}
		if (dram_domain_present()) {
			ret = get_dram_energy_status(&energy_status);
			if (last_dram_energy_status == 0)
				last_dram_energy_status = energy_status;
			if (ret > 0){
				dram_joules = energy_status;
				dram_watts = (energy_status-last_dram_energy_status)/measurment_interval;
			}
			last_dram_energy_status = energy_status;
		}
		if (pp0_domain_present()) {
			ret = get_pp0_energy_status(&energy_status);
			if (last_pp0_energy_status == 0)
				last_pp0_energy_status = energy_status;
			if (ret > 0){
				pp0_joules = energy_status;
				pp0_watts = (energy_status-last_pp0_energy_status)/measurment_interval;
			}
			last_pp0_energy_status = energy_status;
		}
		if (pp1_domain_present()) {
			ret = get_pp1_energy_status(&energy_status);
			if (last_pp1_energy_status == 0)
				last_pp1_energy_status = energy_status;
			if (ret > 0){
				pp1_joules = energy_status;
				pp1_watts = (energy_status-last_pp1_energy_status)/measurment_interval;
			}
			last_pp1_energy_status = energy_status;
		}
		RAPL_DBG_PRINT("%f, %f, %f, %f\n", pkg_watts, dram_watts, pp0_watts, pp1_watts);
		sleep(measurment_interval);
	}
#endif
}
