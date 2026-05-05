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
#include "intel_cpus.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include "../lib.h"
#include "../parameters/parameters.h"
#include "../display.h"

/* List of supported Intel CPU models, sorted by family then model */
static int intel_cpu_models[] = {
	IFM(6, 0x1A),	/* NEHALEM_EP */
	IFM(6, 0x1E),	/* NEHALEM */
	IFM(6, 0x1F),	/* NEHALEM_G */
	IFM(6, 0x25),	/* WESTMERE */
	IFM(6, 0x27),	/* ATOM_SALTWELL_MID */
	IFM(6, 0x2A),	/* SANDYBRIDGE */
	IFM(6, 0x2C),	/* WESTMERE_EP */
	IFM(6, 0x2D),	/* SANDYBRIDGE_X */
	IFM(6, 0x2E),	/* NEHALEM_EX */
	IFM(6, 0x2F),	/* WESTMERE_EX */
	IFM(6, 0x37),	/* ATOM_SILVERMONT */
	IFM(6, 0x3A),	/* IVYBRIDGE */
	IFM(6, 0x3C),	/* HASWELL */
	IFM(6, 0x3D),	/* BROADWELL */
	IFM(6, 0x3E),	/* IVYBRIDGE_X */
	IFM(6, 0x3F),	/* HASWELL_X */
	IFM(6, 0x45),	/* HASWELL_L */
	IFM(6, 0x46),	/* HASWELL_G */
	IFM(6, 0x47),	/* BROADWELL_G */
	IFM(6, 0x4C),	/* ATOM_AIRMONT */
	IFM(6, 0x4D),	/* ATOM_SILVERMONT_D */
	IFM(6, 0x4E),	/* SKYLAKE_L */
	IFM(6, 0x4F),	/* BROADWELL_X */
	IFM(6, 0x55),	/* SKYLAKE_X */
	IFM(6, 0x56),	/* BROADWELL_D */
	IFM(6, 0x5C),	/* ATOM_GOLDMONT */
	IFM(6, 0x5E),	/* SKYLAKE */
	IFM(6, 0x5F),	/* ATOM_GOLDMONT_D */
	IFM(6, 0x66),	/* CANNONLAKE_L */
	IFM(6, 0x6A),	/* ICELAKE_X */
	IFM(6, 0x6C),	/* ICELAKE_D */
	IFM(6, 0x7A),	/* ATOM_GOLDMONT_PLUS */
	IFM(6, 0x7D),	/* ICELAKE */
	IFM(6, 0x7E),	/* ICELAKE_L */
	IFM(6, 0x8A),	/* LAKEFIELD */
	IFM(6, 0x8C),	/* TIGERLAKE_L */
	IFM(6, 0x8D),	/* TIGERLAKE */
	IFM(6, 0x8E),	/* KABYLAKE_L */
	IFM(6, 0x8F),	/* SAPPHIRERAPIDS_X */
	IFM(6, 0x96),	/* ATOM_TREMONT */
	IFM(6, 0x97),	/* ALDERLAKE */
	IFM(6, 0x9A),	/* ALDERLAKE_L */
	IFM(6, 0x9C),	/* ATOM_TREMONT_L */
	IFM(6, 0x9D),	/* ICELAKE_NNPI */
	IFM(6, 0x9E),	/* KABYLAKE */
	IFM(6, 0xA5),	/* COMETLAKE */
	IFM(6, 0xA6),	/* COMETLAKE_L */
	IFM(6, 0xA7),	/* ROCKETLAKE */
	IFM(6, 0xAA),	/* METEORLAKE_L */
	IFM(6, 0xAC),	/* METEORLAKE */
	IFM(6, 0xAD),	/* GRANITERAPIDS_X */
	IFM(6, 0xAE),	/* GRANITERAPIDS_D */
	IFM(6, 0xB5),	/* ARROWLAKE_U */
	IFM(6, 0xB7),	/* RAPTORLAKE */
	IFM(6, 0xBA),	/* RAPTORLAKE_P */
	IFM(6, 0xBD),	/* LUNARLAKE_M */
	IFM(6, 0xBE),	/* ATOM_GRACEMONT */
	IFM(6, 0xBF),	/* RAPTORLAKE_S */
	IFM(6, 0xC5),	/* ARROWLAKE_H */
	IFM(6, 0xC6),	/* ARROWLAKE */
	IFM(6, 0xCC),	/* PANTHERLAKE_L */
	IFM(6, 0xCF),	/* EMERALDRAPIDS_X */
	IFM(6, 0xD5),	/* WILDCATLAKE_L */
	IFM(6, 0xD7),	/* BARTLETTLAKE */
	IFM(6, 0xDD),	/* ATOM_DARKMONT_X */
	IFM(18, 0x01),	/* NOVALAKE */
	IFM(18, 0x03),	/* NOVALAKE_L */
	IFM(19, 0x01),	/* DIAMONDRAPIDS_X */
	0
};

static int intel_pstate_driver_loaded = -1;

int is_supported_intel_cpu(int family, int model, int cpu)
{
	int i;
	uint64_t msr;

	for (i = 0; intel_cpu_models[i] != 0; i++)
		if (IFM(family, model) == intel_cpu_models[i])
			if (cpu < 0 || read_msr(cpu, MSR_APERF, &msr) >= 0)
				return 1;

	return 0;
}

int is_intel_pstate_driver_loaded()
{
	if (intel_pstate_driver_loaded > -1)
		return intel_pstate_driver_loaded;

	std::string scaling_driver = read_sysfs_string("/sys/devices/system/cpu/cpu0/cpufreq/scaling_driver");

	if (scaling_driver == "intel_pstate") {
		intel_pstate_driver_loaded = 1;
	} else {
		intel_pstate_driver_loaded = 0;
	}

	return intel_pstate_driver_loaded;
}

static uint64_t get_msr(int cpu, uint64_t offset)
{
	ssize_t retval;
	uint64_t msr;

	retval = read_msr(cpu, offset, &msr);
	if (retval < 0) {
		reset_display();
		fprintf(stderr, _("read_msr cpu%d 0x%llx : "), cpu, (unsigned long long)offset);
		fprintf(stderr, "%s\n", strerror(errno));
		exit(-2);
	}

	return msr;
}

intel_util::intel_util()
{
	byt_ahci_support=0;
}

void intel_util::byt_has_ahci()
{
	byt_ahci_support = std::filesystem::exists("/sys/bus/pci/devices/0000:00:13.0") ? 1 : 0;
}

int intel_util::get_byt_ahci_support()
{
	return byt_ahci_support;
}

nhm_core::nhm_core(int family, int model)
{
	has_c7_res = 0;

	switch(IFM(family, model)) {
		case IFM(6, 0x2A):	/* SNB */
		case IFM(6, 0x2D):	/* SNB Xeon */
		case IFM(6, 0x3A):      /* IVB */
		case IFM(6, 0x3C):	/* HSW */
		case IFM(6, 0x3D):	/* BDW */
		case IFM(6, 0x3E):      /* IVB Xeon */
		case IFM(6, 0x45):	/* HSW-ULT */
		case IFM(6, 0x4E):	/* SKY */
		case IFM(6, 0x55):	/* SKY-X */
		case IFM(6, 0x5E):	/* SKY */
		case IFM(6, 0x5F):	/* DNV */
		case IFM(6, 0x5C):      /* BXT-P */
		case IFM(6, 0x66):	/* CNL-U/Y */
		case IFM(6, 0x6A):    	/* ICL_X*/
		case IFM(6, 0x7A):	/* GLK */
		case IFM(6, 0x7D):    	/* ICL_DESKTOP */
		case IFM(6, 0x7E):	/* ICL_MOBILE */
		case IFM(6, 0x8A):	/* LKF */
		case IFM(6, 0x8C):	/* TGL_MOBILE */
		case IFM(6, 0x8D):	/* TGL_DESKTOP */
		case IFM(6, 0x8E):	/* KBL_MOBILE */
		case IFM(6, 0x8F):	/* SAPPHIRERAPIDS_X */
		case IFM(6, 0x96):	/* EHL */
		case IFM(6, 0x97):	/* ADL_DESKTOP */
		case IFM(6, 0x9A):	/* ADL_MOBILE */
		case IFM(6, 0x9C):	/* JSL */
		case IFM(6, 0x9D):	/* ICL_NNPI */
		case IFM(6, 0x9E):	/* KBL_DESKTOP */
		case IFM(6, 0xA5):      /* CML_DESKTOP */
		case IFM(6, 0xA6):      /* CML_MOBILE */
		case IFM(6, 0xA7):	/* RKL_DESKTOP */
		case IFM(6, 0xAA):	/* MTL_MOBILE */
		case IFM(6, 0xAC):	/* MTL_DESKTOP */
		case IFM(6, 0xB5):	/* ARL_U */
		case IFM(6, 0xB7):	/* RPL_DESKTOP */
		case IFM(6, 0xBA):	/* RPL_P */
		case IFM(6, 0xBE):	/* ADL_N */
		case IFM(6, 0xBF):	/* RPL_S */
		case IFM(6, 0xC5):	/* ARL_H */
		case IFM(6, 0xC6):	/* ARL_DESKTOP */
		case IFM(6, 0x6C):	/* ICELAKE_D */
		case IFM(6, 0xAD):	/* GRANITERAPIDS_X */
		case IFM(6, 0xAE):	/* GRANITERAPIDS_D */
		case IFM(6, 0xBD):	/* LUNARLAKE_M */
		case IFM(6, 0xCC):	/* PANTHERLAKE_L */
		case IFM(6, 0xCF):	/* EMERALDRAPIDS_X */
		case IFM(6, 0xD5):	/* WILDCATLAKE_L */
		case IFM(6, 0xD7):	/* BARTLETTLAKE */
		case IFM(6, 0xDD):	/* ATOM_DARKMONT_X */
		case IFM(18, 0x01):	/* NOVALAKE */
		case IFM(18, 0x03):	/* NOVALAKE_L */
		case IFM(19, 0x01):	/* DIAMONDRAPIDS_X */
			has_c7_res = 1;
	}

	has_c3_res = 1;
	has_c1_res = 0;

	switch (IFM(family, model)) {
		case IFM(6, 0x37):	/* BYT-M does not support C3/C4 */
		case IFM(6, 0x4C):	/* BSW does not support C3 */
			has_c3_res = 0;
			has_c1_res = 1;
	}

}

void nhm_core::measurement_start(void)
{
	std::string filename;

	/* the abstract function needs to be first since it clears all state */
	abstract_cpu::measurement_start();

	last_stamp = 0;

	if (this->has_c1_res)
		c1_before = get_msr(first_cpu, MSR_CORE_C1_RESIDENCY);
	if (this->has_c3_res)
		c3_before    = get_msr(first_cpu, MSR_CORE_C3_RESIDENCY);
	c6_before    = get_msr(first_cpu, MSR_CORE_C6_RESIDENCY);
	if (this->has_c7_res)
		c7_before    = get_msr(first_cpu, MSR_CORE_C7_RESIDENCY);
	tsc_before   = get_msr(first_cpu, MSR_TSC);

	if (this->has_c1_res)
		insert_cstate("core c1", "C1 (cc1)", 0, c1_before, 1);
	if (this->has_c3_res)
		insert_cstate("core c3", "C3 (cc3)", 0, c3_before, 1);
	insert_cstate("core c6", "C6 (cc6)", 0, c6_before, 1);
	if (this->has_c7_res) {
		insert_cstate("core c7", "C7 (cc7)", 0, c7_before, 1);
	}


	filename = std::format("/sys/devices/system/cpu/cpu{}/cpufreq/stats/time_in_state", first_cpu);

	std::string content = read_file_content(filename);
	if (!content.empty()) {
		std::istringstream stream(content);
		std::string line;
		while (std::getline(stream, line)) {
			uint64_t f;
			std::istringstream iss(line);
			if (iss >> f)
				account_freq(f, 0);
		}
	}
	account_freq(0, 0);

}

void nhm_core::measurement_end(void)
{
	unsigned int i;
	uint64_t time_delta;
	double ratio = 0.0;

	if (this->has_c1_res)
		c1_after = get_msr(first_cpu, MSR_CORE_C1_RESIDENCY);
	if (this->has_c3_res)
		c3_after    = get_msr(first_cpu, MSR_CORE_C3_RESIDENCY);
	c6_after    = get_msr(first_cpu, MSR_CORE_C6_RESIDENCY);
	if (this->has_c7_res)
		c7_after    = get_msr(first_cpu, MSR_CORE_C7_RESIDENCY);
	tsc_after   = get_msr(first_cpu, MSR_TSC);

	if (this->has_c1_res)
		finalize_cstate("core c1", 0, c1_after, 1);
	if (this->has_c3_res)
		finalize_cstate("core c3", 0, c3_after, 1);
	finalize_cstate("core c6", 0, c6_after, 1);
	if (this->has_c7_res)
		finalize_cstate("core c7", 0, c7_after, 1);

	stamp_after = pt_gettime();

	time_factor = 1000000.0 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;

	for (i = 0; i < children.size(); i++)
		if (children[i]) {
			children[i]->measurement_end();
			children[i]->wiggle();
		}

	time_delta = 1000000 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;

	if (tsc_after != tsc_before)
		ratio = 1.0 * time_delta / (tsc_after - tsc_before);

	for (i = 0; i < cstates.size(); i++) {
		struct idle_state *state = cstates[i];

		if (state->after_count == 0)
			continue;

		if (state->after_count != state->before_count)
			continue;

		state->usage_delta =    ratio * (state->usage_after    - state->usage_before)    / state->after_count;
		state->duration_delta = ratio * (state->duration_after - state->duration_before) / state->after_count;
	}

#if 0
	for (i = 0; i < children.size(); i++)
		if (children[i]) {
			for (j = 0; j < children[i]->pstates.size(); j++) {
				class frequency *state;
				state = children[i]->pstates[j];
				if (!state)
					continue;

				update_pstate(  state->freq, state->human_name.c_str(), state->time_before, state->before_count);
				finalize_pstate(state->freq,                    state->time_after,  state->after_count);
			}
		}
#endif
	total_stamp = 0;
}

#include <format>

std::string nhm_core::fill_pstate_line(int line_nr)
{
	const int intel_pstate = is_intel_pstate_driver_loaded();
	unsigned int i;

	if (!intel_pstate && total_stamp ==0) {
		for (i = 0; i < pstates.size(); i++)
			total_stamp += pstates[i]->time_after;
		if (total_stamp == 0)
			total_stamp = 1;
	}

	if (line_nr == LEVEL_HEADER) {
		return _("  Core");
	}

	if (intel_pstate > 0 || line_nr >= (int)pstates.size() || line_nr < 0)
		return "";

	return std::format(" {:5.1f}% ", percentage(1.0* (pstates[line_nr]->time_after) / total_stamp));
}

nhm_package::nhm_package(int family, int model)
{
	has_c8c9c10_res = 0;
	has_c2c6_res = 0;
	has_c7_res = 0;
	has_c6c_res = 0;

	switch(IFM(family, model)) {
		case IFM(6, 0x2A):	/* SNB */
		case IFM(6, 0x2D):	/* SNB Xeon */
		case IFM(6, 0x3A):      /* IVB */
		case IFM(6, 0x3C):	/* HSW */
		case IFM(6, 0x3D):	/* BDW */
		case IFM(6, 0x3E):      /* IVB Xeon */
		case IFM(6, 0x45):	/* HSW-ULT */
		case IFM(6, 0x4E):	/* SKY */
		case IFM(6, 0x55):	/* SKY-X */
		case IFM(6, 0x5C):	/* BXT-P */
		case IFM(6, 0x5E):	/* SKY */
		case IFM(6, 0x5F):	/* DNV */
		case IFM(6, 0x66):	/* CNL-U/Y */
		case IFM(6, 0x6A):  	/* ICL_X*/
		case IFM(6, 0x7A):	/* GLK */
		case IFM(6, 0x7D):    	/* ICL_DESKTOP */
		case IFM(6, 0x7E):	/* ICL_MOBILE */
		case IFM(6, 0x8A):	/* LKF */
		case IFM(6, 0x8C):	/* TGL_MOBILE */
		case IFM(6, 0x8D):	/* TGL_DESKTOP */
		case IFM(6, 0x8E):	/* KBL_MOBILE */
		case IFM(6, 0x8F):	/* SAPPHIRERAPIDS_X */
		case IFM(6, 0x96):      /* EHL */
		case IFM(6, 0x97):	/* ADL_DESKTOP */
		case IFM(6, 0x9A):	/* ADL_MOBILE */
		case IFM(6, 0x9C):	/* JSL */
		case IFM(6, 0x9D):	/* ICL_NNPI */
		case IFM(6, 0x9E):	/* KBL_DESKTOP */
		case IFM(6, 0xA5):      /* CML_DESKTOP */
		case IFM(6, 0xA6):      /* CML_MOBILE */
		case IFM(6, 0xA7):	/* RKL_DESKTOP */
		case IFM(6, 0xAA):	/* MTL_MOBILE */
		case IFM(6, 0xAC):	/* MTL_DESKTOP */
		case IFM(6, 0xB5):	/* ARL_U */
		case IFM(6, 0xB7):	/* RPL_DESKTOP */
		case IFM(6, 0xBA):	/* RPL_P */
		case IFM(6, 0xBE):	/* ADL_N */
		case IFM(6, 0xBF):	/* RPL_S */
		case IFM(6, 0xC5):	/* ARL_H */
		case IFM(6, 0xC6):	/* ARL_DESKTOP */
		case IFM(6, 0x6C):	/* ICELAKE_D */
		case IFM(6, 0xAD):	/* GRANITERAPIDS_X */
		case IFM(6, 0xAE):	/* GRANITERAPIDS_D */
		case IFM(6, 0xBD):	/* LUNARLAKE_M */
		case IFM(6, 0xCC):	/* PANTHERLAKE_L */
		case IFM(6, 0xCF):	/* EMERALDRAPIDS_X */
		case IFM(6, 0xD5):	/* WILDCATLAKE_L */
		case IFM(6, 0xD7):	/* BARTLETTLAKE */
		case IFM(6, 0xDD):	/* ATOM_DARKMONT_X */
		case IFM(18, 0x01):	/* NOVALAKE */
		case IFM(18, 0x03):	/* NOVALAKE_L */
		case IFM(19, 0x01):	/* DIAMONDRAPIDS_X */
			has_c2c6_res=1;
			has_c7_res = 1;
	}

	has_c3_res = 1;

	switch(IFM(family, model)) {
		/* BYT-M doesn't have C3 or C7 */
		/* BYT-T doesn't have C3 but it has C7 */
		case IFM(6, 0x37):
			has_c2c6_res=1;
			this->byt_has_ahci();
			if ((this->get_byt_ahci_support()) == 0)
				has_c7_res = 1;/*BYT-T PC7 <- S0iX*/
			else
				has_c7_res = 0;
			break;
		case IFM(6, 0x4C): /* BSW doesn't have C3 */
			has_c3_res = 0;
			has_c6c_res = 1; /* BSW only exposes package C6 */
			break;
	}

	/*Has C8/9/10*/
	switch(IFM(family, model)) {
		case IFM(6, 0x3D):	/* BDW */
		case IFM(6, 0x45):	/* HSW */
		case IFM(6, 0x4E):	/* SKY */
		case IFM(6, 0x5C):	/* BXT-P */
		case IFM(6, 0x5E):	/* SKY */
		case IFM(6, 0x5F):	/* DNV */
		case IFM(6, 0x66):	/* CNL-U/Y */
		case IFM(6, 0x7A):	/* GLK */
		case IFM(6, 0x7D):	/* ICL_DESKTOP */
		case IFM(6, 0x7E):	/* ICL_MOBILE */
		case IFM(6, 0x8A):	/* LKF */
		case IFM(6, 0x8C):	/* TGL_MOBILE */
		case IFM(6, 0x8D):	/* TGL_DESKTOP */
		case IFM(6, 0x8E):	/* KBL_MOBILE */
		case IFM(6, 0x96):      /* EHL */
		case IFM(6, 0x97):	/* ADL_DESKTOP */
		case IFM(6, 0x9A):	/* ADL_MOBILE */
		case IFM(6, 0x9C):	/* JSL */
		case IFM(6, 0x9D):	/* ICL_NNPI */
		case IFM(6, 0x9E):	/* KBL_DESKTOP */
		case IFM(6, 0xA5):      /* CML_DESKTOP */
		case IFM(6, 0xA6):      /* CML_MOBILE */
		case IFM(6, 0xA7):	/* RKL_DESKTOP */
		case IFM(6, 0xAA):	/* MTL_MOBILE */
		case IFM(6, 0xAC):	/* MTL_DESKTOP */
		case IFM(6, 0xB5):	/* ARL_U */
		case IFM(6, 0xB7):	/* RPL_DESKTOP */
		case IFM(6, 0xBA):	/* RPL_P */
		case IFM(6, 0xBE):	/* ADL_N */
		case IFM(6, 0xBF):	/* RPL_S */
		case IFM(6, 0xC5):	/* ARL_H */
		case IFM(6, 0xC6):	/* ARL_DESKTOP */
		case IFM(6, 0xBD):	/* LUNARLAKE_M */
		case IFM(6, 0xCC):	/* PANTHERLAKE_L */
		case IFM(6, 0xD5):	/* WILDCATLAKE_L */
		case IFM(6, 0xD7):	/* BARTLETTLAKE */
		case IFM(18, 0x01):	/* NOVALAKE */
		case IFM(18, 0x03):	/* NOVALAKE_L */
			has_c8c9c10_res = 1;
			break;
	}
}

std::string nhm_package::fill_pstate_line(int line_nr)
{
	const int intel_pstate = is_intel_pstate_driver_loaded();
	unsigned int i;

	if (!intel_pstate && total_stamp ==0) {
		for (i = 0; i < pstates.size(); i++)
			total_stamp += pstates[i]->time_after;
		if (total_stamp == 0)
			total_stamp = 1;
	}


	if (line_nr == LEVEL_HEADER) {
		return _("  Package");
	}

	if (intel_pstate > 0 || line_nr >= (int)pstates.size() || line_nr < 0)
		return "";

	return std::format(" {:5.1f}% ", percentage(1.0* (pstates[line_nr]->time_after) / total_stamp));
}



void nhm_package::measurement_start(void)
{
	abstract_cpu::measurement_start();

	last_stamp = 0;

	if (this->has_c2c6_res)
		c2_before    = get_msr(number, MSR_PKG_C2_RESIDENCY);

	if (this->has_c3_res)
		c3_before    = get_msr(number, MSR_PKG_C3_RESIDENCY);

	/*
	 * Hack for Braswell where C7 MSR is actually BSW C6
	 */
	if (this->has_c6c_res)
		c6_before    = get_msr(number, MSR_PKG_C7_RESIDENCY);
	else
		c6_before    = get_msr(number, MSR_PKG_C6_RESIDENCY);

	if (this->has_c7_res)
		c7_before    = get_msr(number, MSR_PKG_C7_RESIDENCY);
	if (this->has_c8c9c10_res) {
		c8_before    = get_msr(number, MSR_PKG_C8_RESIDENCY);
		c9_before    = get_msr(number, MSR_PKG_C9_RESIDENCY);
		c10_before    = get_msr(number, MSR_PKG_C10_RESIDENCY);
	}
	tsc_before   = get_msr(first_cpu, MSR_TSC);

	if (this->has_c2c6_res)
		insert_cstate("pkg c2", "C2 (pc2)", 0, c2_before, 1);

	if (this->has_c3_res)
		insert_cstate("pkg c3", "C3 (pc3)", 0, c3_before, 1);
	insert_cstate("pkg c6", "C6 (pc6)", 0, c6_before, 1);
	if (this->has_c7_res)
		insert_cstate("pkg c7", "C7 (pc7)", 0, c7_before, 1);
	if (this->has_c8c9c10_res) {
		insert_cstate("pkg c8", "C8 (pc8)", 0, c8_before, 1);
		insert_cstate("pkg c9", "C9 (pc9)", 0, c9_before, 1);
		insert_cstate("pkg c10", "C10 (pc10)", 0, c10_before, 1);
	}
}

void nhm_package::measurement_end(void)
{
	uint64_t time_delta;
	double ratio = 0.0;
	unsigned int i, j;

	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->wiggle();


	if (this->has_c2c6_res)
		c2_after    = get_msr(number, MSR_PKG_C2_RESIDENCY);

	if (this->has_c3_res)
		c3_after    = get_msr(number, MSR_PKG_C3_RESIDENCY);

	if (this->has_c6c_res)
		c6_after    = get_msr(number, MSR_PKG_C7_RESIDENCY);
	else
		c6_after    = get_msr(number, MSR_PKG_C6_RESIDENCY);

	if (this->has_c7_res)
		c7_after    = get_msr(number, MSR_PKG_C7_RESIDENCY);
	if (has_c8c9c10_res) {
		c8_after = get_msr(number, MSR_PKG_C8_RESIDENCY);
		c9_after = get_msr(number, MSR_PKG_C9_RESIDENCY);
		c10_after = get_msr(number, MSR_PKG_C10_RESIDENCY);
	}
	tsc_after   = get_msr(first_cpu, MSR_TSC);

	stamp_after = pt_gettime();

	time_factor = 1000000.0 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;


	if (this->has_c2c6_res)
		finalize_cstate("pkg c2", 0, c2_after, 1);

	if (this->has_c3_res)
		finalize_cstate("pkg c3", 0, c3_after, 1);
	finalize_cstate("pkg c6", 0, c6_after, 1);
	if (this->has_c7_res)
		finalize_cstate("pkg c7", 0, c7_after, 1);
	if (has_c8c9c10_res) {
		finalize_cstate("pkg c8", 0, c8_after, 1);
		finalize_cstate("pkg c9", 0, c9_after, 1);
		finalize_cstate("pkg c10", 0, c10_after, 1);
	}

	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->measurement_end();

	time_delta = 1000000 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;

	if (tsc_after != tsc_before)
		ratio = 1.0 * time_delta / (tsc_after - tsc_before);


	for (i = 0; i < cstates.size(); i++) {
		struct idle_state *state = cstates[i];

		if (state->after_count == 0)
			continue;

		if (state->after_count != state->before_count)
			continue;

		state->usage_delta =    ratio * (state->usage_after    - state->usage_before)    / state->after_count;
		state->duration_delta = ratio * (state->duration_after - state->duration_before) / state->after_count;
	}
	for (i = 0; i < children.size(); i++)
		if (children[i]) {
			for (j = 0; j < children[i]->pstates.size(); j++) {
				class frequency *state;
				state = children[i]->pstates[j];
				if (!state)
					continue;

				update_pstate(  state->freq, state->human_name.c_str(), state->time_before, state->before_count);
				finalize_pstate(state->freq,                    state->time_after,  state->after_count);
			}
		}
	total_stamp = 0;

}

void nhm_cpu::measurement_start(void)
{
	std::string filename;

	cpu_linux::measurement_start();

	last_stamp = 0;

	aperf_before = get_msr(number, MSR_APERF);
	mperf_before = get_msr(number, MSR_MPERF);
	tsc_before   = get_msr(number, MSR_TSC);

	insert_cstate("active", _("C0 active"), 0, aperf_before, 1);

	filename = std::format("/sys/devices/system/cpu/cpu{}/cpufreq/stats/time_in_state", first_cpu);

	std::string content = read_file_content(filename);
	if (!content.empty()) {
		std::istringstream stream(content);
		std::string line;
		while (std::getline(stream, line)) {
			uint64_t f;
			std::istringstream iss(line);
			if (iss >> f)
				account_freq(f, 0);
		}
	}
	account_freq(0, 0);
}

void nhm_cpu::measurement_end(void)
{
	uint64_t time_delta;
	double ratio = 0.0;
	unsigned int i;

	aperf_after = get_msr(number, MSR_APERF);
	mperf_after = get_msr(number, MSR_MPERF);
	tsc_after   = get_msr(number, MSR_TSC);



	finalize_cstate("active", 0, aperf_after, 1);


	cpu_linux::measurement_end();

	time_delta = 1000000 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;

	if (tsc_after != tsc_before)
		ratio = 1.0 * time_delta / (tsc_after - tsc_before);


	for (i = 0; i < cstates.size(); i++) {
		struct idle_state *state = cstates[i];
		if (state->line_level != LEVEL_C0)
			continue;

		state->usage_delta =    ratio * (state->usage_after    - state->usage_before)    / state->after_count;
		state->duration_delta = ratio * (state->duration_after - state->duration_before) / state->after_count;
	}

	total_stamp = 0;

}

std::string nhm_cpu::fill_pstate_name(int line_nr)
{
	if (line_nr == LEVEL_C0) {
		return _("Average");
	}
	return cpu_linux::fill_pstate_name(line_nr);
}

std::string nhm_cpu::fill_pstate_line(int line_nr)
{
	const int intel_pstate = is_intel_pstate_driver_loaded();

	if (!intel_pstate && total_stamp ==0) {
		unsigned int i;
		for (i = 0; i < pstates.size(); i++)
			total_stamp += pstates[i]->time_after;
		if (total_stamp == 0)
			total_stamp = 1;
	}

	if (line_nr == LEVEL_HEADER) {
		return pt_format(_(" CPU {}"), number);
	}

	if (line_nr == LEVEL_C0) {
		double F;
		if (mperf_after <= mperf_before || time_factor < 1.0)
			return "";
		F = 1.0 * (tsc_after - tsc_before) * (aperf_after - aperf_before) / (mperf_after - mperf_before) / time_factor * 1000;
		return hz_to_human(F, 1);
	}
	if (intel_pstate > 0 || line_nr >= (int)pstates.size() || line_nr < 0)
		return "";

	return std::format(" {:5.1f}% ", percentage(1.0* (pstates[line_nr]->time_after) / total_stamp));
}


int nhm_cpu::has_pstate_level(int level)
{
	if (level == LEVEL_C0)
		return 1;
	return cpu_linux::has_pstate_level(level);
}

void nhm_package::collect_json_fields(std::string &_js)
{
    abstract_cpu::collect_json_fields(_js);
    JSON_FIELD(has_c7_res);
    JSON_FIELD(has_c2c6_res);
    JSON_FIELD(has_c3_res);
    JSON_FIELD(has_c6c_res);
    JSON_FIELD(has_c8c9c10_res);
    JSON_FIELD(c2_before);
    JSON_FIELD(c2_after);
    JSON_FIELD(c3_before);
    JSON_FIELD(c3_after);
    JSON_FIELD(c6_before);
    JSON_FIELD(c6_after);
    JSON_FIELD(c7_before);
    JSON_FIELD(c7_after);
    JSON_FIELD(c8_before);
    JSON_FIELD(c8_after);
    JSON_FIELD(c9_before);
    JSON_FIELD(c9_after);
    JSON_FIELD(c10_before);
    JSON_FIELD(c10_after);
    JSON_FIELD(tsc_before);
    JSON_FIELD(tsc_after);
    JSON_FIELD(last_stamp);
    JSON_FIELD(total_stamp);
}

void nhm_core::collect_json_fields(std::string &_js)
{
    abstract_cpu::collect_json_fields(_js);
    JSON_FIELD(has_c1_res);
    JSON_FIELD(has_c7_res);
    JSON_FIELD(has_c3_res);
    JSON_FIELD(c1_before);
    JSON_FIELD(c1_after);
    JSON_FIELD(c3_before);
    JSON_FIELD(c3_after);
    JSON_FIELD(c6_before);
    JSON_FIELD(c6_after);
    JSON_FIELD(c7_before);
    JSON_FIELD(c7_after);
    JSON_FIELD(tsc_before);
    JSON_FIELD(tsc_after);
    JSON_FIELD(last_stamp);
    JSON_FIELD(total_stamp);
}

void nhm_cpu::collect_json_fields(std::string &_js)
{
    abstract_cpu::collect_json_fields(_js);
    JSON_FIELD(aperf_before);
    JSON_FIELD(aperf_after);
    JSON_FIELD(mperf_before);
    JSON_FIELD(mperf_after);
    JSON_FIELD(tsc_before);
    JSON_FIELD(tsc_after);
    JSON_FIELD(last_stamp);
    JSON_FIELD(total_stamp);
}

void i965_core::collect_json_fields(std::string &_js)
{
    abstract_cpu::collect_json_fields(_js);
    JSON_FIELD(rc6_before);
    JSON_FIELD(rc6_after);
    JSON_FIELD(rc6p_before);
    JSON_FIELD(rc6p_after);
    JSON_FIELD(rc6pp_before);
    JSON_FIELD(rc6pp_after);
    JSON_KV("before_sec", (long)before.tv_sec);
    JSON_KV("before_usec", (long)before.tv_usec);
    JSON_KV("after_sec", (long)after.tv_sec);
    JSON_KV("after_usec", (long)after.tv_usec);
}
