/*
 * Stubs for c_rapl_interface — used when testing the real
 * cpu_rapl_device, dram_rapl_device, and gpu_rapl_device implementations.
 *
 * Domain-present flags and per-domain energy queues are test-configurable.
 * Defaults (all false, empty queues) reproduce the "no domain present" path.
 */
#include <string>
#include <cstdint>
#include <cassert>
#include <cstdio>

#include "cpu/rapl/rapl_interface.h"

/* ── Test-configurable knobs ─────────────────────────────────────── */

bool rapl_pp0_present  = false;
bool rapl_dram_present = false;
bool rapl_pp1_present  = false;

/* Separate queues per domain — prevents wrong-getter calls from passing */
static double pp0_seq[8]  = {}; static int pp0_idx = 0, pp0_cnt = 0;
static double dram_seq[8] = {}; static int dram_idx = 0, dram_cnt = 0;
static double pp1_seq[8]  = {}; static int pp1_idx = 0, pp1_cnt = 0;

void rapl_stub_reset()
{
    rapl_pp0_present = rapl_dram_present = rapl_pp1_present = false;
    pp0_idx = pp0_cnt = dram_idx = dram_cnt = pp1_idx = pp1_cnt = 0;
}

void rapl_push_pp0_energy(double v)  { assert(pp0_cnt  < 8); pp0_seq[pp0_cnt++]   = v; }
void rapl_push_dram_energy(double v) { assert(dram_cnt < 8); dram_seq[dram_cnt++] = v; }
void rapl_push_pp1_energy(double v)  { assert(pp1_cnt  < 8); pp1_seq[pp1_cnt++]   = v; }

/* ── c_rapl_interface stubs ──────────────────────────────────────── */

c_rapl_interface::c_rapl_interface(
    [[maybe_unused]] const std::string &dev_name,
    [[maybe_unused]] int cpu) {}

bool c_rapl_interface::pkg_domain_present()  { return false; }
bool c_rapl_interface::dram_domain_present() { return rapl_dram_present; }
bool c_rapl_interface::pp0_domain_present()  { return rapl_pp0_present; }
bool c_rapl_interface::pp1_domain_present()  { return rapl_pp1_present; }

int c_rapl_interface::read_msr(
    [[maybe_unused]] int cpu,
    [[maybe_unused]] unsigned int idx,
    [[maybe_unused]] uint64_t *val) { return -1; }

int c_rapl_interface::write_msr(
    [[maybe_unused]] int cpu,
    [[maybe_unused]] unsigned int idx,
    [[maybe_unused]] uint64_t val)  { return -1; }

int c_rapl_interface::get_rapl_power_unit([[maybe_unused]] uint64_t *v) { return -1; }
double c_rapl_interface::get_power_unit()         { return 0.0; }
double c_rapl_interface::get_energy_status_unit() { return 0.0; }
double c_rapl_interface::get_time_unit()          { return 0.0; }

int c_rapl_interface::get_pkg_energy_status([[maybe_unused]] double *s) { return -1; }
int c_rapl_interface::get_pkg_power_info(
    [[maybe_unused]] double *a, [[maybe_unused]] double *b,
    [[maybe_unused]] double *c, [[maybe_unused]] double *d) { return -1; }
int c_rapl_interface::get_pkg_power_limit([[maybe_unused]] uint64_t *v) { return -1; }
int c_rapl_interface::set_pkg_power_limit([[maybe_unused]] uint64_t v)  { return -1; }

int c_rapl_interface::get_dram_energy_status(double *s) {
    assert(dram_idx < dram_cnt && "dram energy queue exhausted — unexpected call");
    *s = dram_seq[dram_idx++]; return 0;
}
int c_rapl_interface::get_dram_power_info(
    [[maybe_unused]] double *a, [[maybe_unused]] double *b,
    [[maybe_unused]] double *c, [[maybe_unused]] double *d) { return -1; }
int c_rapl_interface::get_dram_power_limit([[maybe_unused]] uint64_t *v) { return -1; }
int c_rapl_interface::set_dram_power_limit([[maybe_unused]] uint64_t v)  { return -1; }

int c_rapl_interface::get_pp0_energy_status(double *s) {
    assert(pp0_idx < pp0_cnt && "pp0 energy queue exhausted — unexpected call");
    *s = pp0_seq[pp0_idx++]; return 0;
}
int c_rapl_interface::get_pp0_power_limit([[maybe_unused]] uint64_t *v) { return -1; }
int c_rapl_interface::set_pp0_power_limit([[maybe_unused]] uint64_t v)  { return -1; }
int c_rapl_interface::get_pp0_power_policy([[maybe_unused]] unsigned int *p) { return -1; }

int c_rapl_interface::get_pp1_energy_status(double *s) {
    assert(pp1_idx < pp1_cnt && "pp1 energy queue exhausted — unexpected call");
    *s = pp1_seq[pp1_idx++]; return 0;
}
int c_rapl_interface::get_pp1_power_limit([[maybe_unused]] uint64_t *v) { return -1; }
int c_rapl_interface::set_pp1_power_limit([[maybe_unused]] uint64_t v)  { return -1; }
int c_rapl_interface::get_pp1_power_policy([[maybe_unused]] unsigned int *p) { return -1; }

void c_rapl_interface::rapl_measure_energy() {}

