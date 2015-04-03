#ifndef PowerTop_INTEL_CPUS_H_84F09FB4F519470FA914AA9B02453221
#define PowerTop_INTEL_CPUS_H_84F09FB4F519470FA914AA9B02453221
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
	int byt_ahci_support;
        DIR *dir;
public:
	intel_util();
	virtual void byt_has_ahci();
	virtual int get_byt_ahci_support();
};

class nhm_package: public cpu_package, public intel_util
{
private:
	uint64_t	c2_before, c2_after;
	uint64_t	c3_before, c3_after;
	uint64_t	c6_before, c6_after;
	uint64_t	c7_before, c7_after;
	uint64_t	c8_before, c8_after;
	uint64_t	c9_before, c9_after;
	uint64_t	c10_before, c10_after;
	uint64_t	tsc_before, tsc_after;

	uint64_t	last_stamp;
	uint64_t	total_stamp;
public:
	int		has_c7_res;
	int		has_c2c6_res;
	int		has_c3_res;
	int		has_c6c_res;		/* BSW */
	int		has_c8c9c10_res;
	nhm_package(int model);
	virtual void	measurement_start(void);
	virtual void	measurement_end(void);
	virtual int     can_collapse(void) { return 0;};

	virtual char *  fill_pstate_line(int line_nr, char *buffer);
};

class nhm_core: public cpu_core, public intel_util
{
private:
	uint64_t	c1_before, c1_after;
	uint64_t	c3_before, c3_after;
	uint64_t	c6_before, c6_after;
	uint64_t	c7_before, c7_after;
	uint64_t	tsc_before, tsc_after;

	uint64_t	last_stamp;
	uint64_t	total_stamp;
public:
	int		has_c1_res;
	int		has_c7_res;
	int		has_c3_res;
	nhm_core(int model);
	virtual void	measurement_start(void);
	virtual void	measurement_end(void);
	virtual int     can_collapse(void) { return 0;};

	virtual char *  fill_pstate_line(int line_nr, char *buffer);
};

class nhm_cpu: public cpu_linux, public intel_util
{
private:
	uint64_t	aperf_before;
	uint64_t	aperf_after;
	uint64_t	mperf_before;
	uint64_t	mperf_after;
	uint64_t	tsc_before, tsc_after;

	uint64_t	last_stamp;
	uint64_t	total_stamp;
public:
	virtual void	measurement_start(void);
	virtual void	measurement_end(void);
	virtual int     can_collapse(void) { return 0;};

	virtual char *  fill_pstate_name(int line_nr, char *buffer);
	virtual char *  fill_pstate_line(int line_nr, char *buffer);
	virtual int	has_pstate_level(int level);
};

class atom_package: public cpu_package
{
public:
	virtual void	measurement_start(void);
	virtual void	measurement_end(void);

};

class atom_core: public cpu_core
{
public:
	virtual void	measurement_start(void);
	virtual void	measurement_end(void);

};


class i965_core: public cpu_core
{
private:
	uint64_t	rc6_before, rc6_after;
	uint64_t	rc6p_before, rc6p_after;
	uint64_t	rc6pp_before, rc6pp_after;

	struct timeval	before;
	struct timeval	after;

public:
	virtual void	measurement_start(void);
	virtual void	measurement_end(void);
	virtual int     can_collapse(void) { return 0;};

	virtual char *  fill_pstate_line(int line_nr, char *buffer);
	virtual char *  fill_pstate_name(int line_nr, char *buffer);
	virtual char *  fill_cstate_line(int line_nr, char *buffer, const char *separator);
	virtual int	has_pstate_level(int level) { return 0; };
	virtual int	has_pstates(void) { return 0; };
	virtual void	wiggle(void) { };

};

int is_supported_intel_cpu(int model);
int byt_has_ahci();

#endif
