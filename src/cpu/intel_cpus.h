#pragma once
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
#include <stdint.h>
#include <sys/time.h>
#include <dirent.h>

#include "cpu.h"
#include "../lib.h"


#define MSR_TSC				0x10
#define MSR_NEHALEM_PLATFORM_INFO	0xCE
#define MSR_NEHALEM_TURBO_RATIO_LIMIT	0x1AD
#define MSR_APERF			0xE8
#define MSR_MPERF			0xE7
#define MSR_PKG_C2_RESIDENCY		0x60D
#define MSR_PKG_C3_RESIDENCY		0x3F8
#define MSR_PKG_C6_RESIDENCY		0x3F9
#define MSR_PKG_C7_RESIDENCY		0x3FA
#define MSR_PKG_C8_RESIDENCY		0x630
#define MSR_PKG_C9_RESIDENCY		0x631
#define MSR_PKG_C10_RESIDENCY		0x632
#define MSR_CORE_C1_RESIDENCY		0x660
#define MSR_CORE_C3_RESIDENCY		0x3FC
#define MSR_CORE_C6_RESIDENCY		0x3FD
#define MSR_CORE_C7_RESIDENCY		0x3FE

class intel_util
{
protected:
	int byt_ahci_support = 0;
        DIR *dir = nullptr;
public:
	intel_util();
	virtual void byt_has_ahci();
	virtual int get_byt_ahci_support();
};

class nhm_package: public cpu_package, public intel_util
{
private:
	uint64_t	c2_before = 0, c2_after = 0;
	uint64_t	c3_before = 0, c3_after = 0;
	uint64_t	c6_before = 0, c6_after = 0;
	uint64_t	c7_before = 0, c7_after = 0;
	uint64_t	c8_before = 0, c8_after = 0;
	uint64_t	c9_before = 0, c9_after = 0;
	uint64_t	c10_before = 0, c10_after = 0;
	uint64_t	tsc_before = 0, tsc_after = 0;

	uint64_t	last_stamp = 0;
	uint64_t	total_stamp = 0;
public:
	int		has_c7_res = 0;
	int		has_c2c6_res = 0;
	int		has_c3_res = 0;
	int		has_c6c_res = 0;		/* BSW */
	int		has_c8c9c10_res = 0;
	nhm_package(int model);
	virtual void	measurement_start(void) override;
	virtual void	measurement_end(void) override;
	virtual int     can_collapse(void) override { return 0;};

	virtual std::string  fill_pstate_line(int line_nr) override;
	void collect_json_fields(std::string &_js) override;
};

class nhm_core: public cpu_core, public intel_util
{
private:
	uint64_t	c1_before = 0, c1_after = 0;
	uint64_t	c3_before = 0, c3_after = 0;
	uint64_t	c6_before = 0, c6_after = 0;
	uint64_t	c7_before = 0, c7_after = 0;
	uint64_t	tsc_before = 0, tsc_after = 0;

	uint64_t	last_stamp = 0;
	uint64_t	total_stamp = 0;
public:
	int		has_c1_res = 0;
	int		has_c7_res = 0;
	int		has_c3_res = 0;
	nhm_core(int model);
	virtual void	measurement_start(void) override;
	virtual void	measurement_end(void) override;
	virtual int     can_collapse(void) override { return 0;};

	virtual std::string  fill_pstate_line(int line_nr) override;
	void collect_json_fields(std::string &_js) override;
};

class nhm_cpu: public cpu_linux, public intel_util
{
private:
	uint64_t	aperf_before = 0;
	uint64_t	aperf_after = 0;
	uint64_t	mperf_before = 0;
	uint64_t	mperf_after = 0;
	uint64_t	tsc_before = 0, tsc_after = 0;

	uint64_t	last_stamp = 0;
	uint64_t	total_stamp = 0;
public:
	virtual void	measurement_start(void) override;
	virtual void	measurement_end(void) override;
	virtual int     can_collapse(void) override { return 0;};

	virtual std::string  fill_pstate_name(int line_nr) override;
	virtual std::string  fill_pstate_line(int line_nr) override;
	virtual int	has_pstate_level(int level) override;
	void collect_json_fields(std::string &_js) override;
};

class atom_package: public cpu_package
{
public:
	virtual void	measurement_start(void) override;
	virtual void	measurement_end(void) override;

};

class atom_core: public cpu_core
{
public:
	virtual void	measurement_start(void) override;
	virtual void	measurement_end(void) override;

};


class i965_core: public cpu_core
{
private:
	uint64_t	rc6_before = 0, rc6_after = 0;
	uint64_t	rc6p_before = 0, rc6p_after = 0;
	uint64_t	rc6pp_before = 0, rc6pp_after = 0;

	struct timeval	before;
	struct timeval	after;

public:
	virtual void	measurement_start(void) override;
	virtual void	measurement_end(void) override;
	virtual int     can_collapse(void) override { return 0;};

	virtual std::string  fill_pstate_line(int line_nr) override;
	virtual std::string  fill_pstate_name(int line_nr) override;
	virtual std::string  fill_cstate_line(int line_nr, const std::string &separator) override;
	virtual int	has_pstate_level([[maybe_unused]] int level) override { return 0; };
	virtual int	has_pstates(void) override { return 0; };
	virtual void	wiggle(void) override { };
	void collect_json_fields(std::string &_js) override;

};

int is_supported_intel_cpu(int model, int cpu);
int byt_has_ahci();

int is_intel_pstate_driver_loaded();

