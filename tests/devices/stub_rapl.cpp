/*
 * Minimal stubs for c_rapl_interface and gpu_rapl_device needed when linking
 * i915-gpu.cpp in tests. create_i915_gpu() references gpu_rapl_device, which
 * in turn has a c_rapl_interface member. These stubs prevent any RAPL hardware
 * access and allow the i915gpu tests to run without RAPL support.
 */
#include <string>
#include <cstdint>

#include "cpu/rapl/rapl_interface.h"
#include "devices/gpu_rapl_device.h"

/* ── c_rapl_interface stubs ──────────────────────────────────────── */

c_rapl_interface::c_rapl_interface(
    [[maybe_unused]] const std::string &dev_name,
    [[maybe_unused]] int cpu) {}

bool c_rapl_interface::pkg_domain_present()  { return false; }
bool c_rapl_interface::dram_domain_present() { return false; }
bool c_rapl_interface::pp0_domain_present()  { return false; }
bool c_rapl_interface::pp1_domain_present()  { return false; }

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

int c_rapl_interface::get_dram_energy_status([[maybe_unused]] double *s) { return -1; }
int c_rapl_interface::get_dram_power_info(
    [[maybe_unused]] double *a, [[maybe_unused]] double *b,
    [[maybe_unused]] double *c, [[maybe_unused]] double *d) { return -1; }
int c_rapl_interface::get_dram_power_limit([[maybe_unused]] uint64_t *v) { return -1; }
int c_rapl_interface::set_dram_power_limit([[maybe_unused]] uint64_t v)  { return -1; }

int c_rapl_interface::get_pp0_energy_status([[maybe_unused]] double *s) { return -1; }
int c_rapl_interface::get_pp0_power_limit([[maybe_unused]] uint64_t *v) { return -1; }
int c_rapl_interface::set_pp0_power_limit([[maybe_unused]] uint64_t v)  { return -1; }
int c_rapl_interface::get_pp0_power_policy([[maybe_unused]] unsigned int *p) { return -1; }

int c_rapl_interface::get_pp1_energy_status([[maybe_unused]] double *s) { return -1; }
int c_rapl_interface::get_pp1_power_limit([[maybe_unused]] uint64_t *v) { return -1; }
int c_rapl_interface::set_pp1_power_limit([[maybe_unused]] uint64_t v)  { return -1; }
int c_rapl_interface::get_pp1_power_policy([[maybe_unused]] unsigned int *p) { return -1; }

void c_rapl_interface::rapl_measure_energy() {}

/* ── gpu_rapl_device stubs ───────────────────────────────────────── */

gpu_rapl_device::gpu_rapl_device([[maybe_unused]] i915gpu *parent)
    : i915gpu(), rapl()
{
    device_valid = false;
}

void gpu_rapl_device::start_measurement()  {}
void gpu_rapl_device::end_measurement()    {}

double gpu_rapl_device::power_usage(
    [[maybe_unused]] struct result_bundle *result,
    [[maybe_unused]] struct parameter_bundle *bundle) { return 0.0; }

void gpu_rapl_device::collect_json_fields([[maybe_unused]] std::string &_js) {}
