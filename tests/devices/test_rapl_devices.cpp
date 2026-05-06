#include <string>
#include <cmath>

#include "cpu/cpu_rapl_device.h"
#include "cpu/dram_rapl_device.h"
#include "cpu/cpudevice.h"
#include "devices/gpu_rapl_device.h"
#include "test_framework.h"
#include "../test_helper.h"

void (*ui_notify_user)(const std::string &) = nullptr;

static const std::string DATA_DIR = TEST_DATA_DIR;

/* Stub helpers declared in stub_rapl_iface.cpp */
extern bool rapl_pp0_present;
extern bool rapl_dram_present;
extern bool rapl_pp1_present;
void rapl_stub_reset();
void rapl_push_pp0_energy(double v);
void rapl_push_dram_energy(double v);
void rapl_push_pp1_energy(double v);

static void reset_replay()
{
    test_framework_manager::get().reset();
}

static void begin_replay(const std::string &fixture)
{
    test_framework_manager::get().reset();
    test_framework_manager::get().set_replay(fixture);
}

/* ── cpu_rapl_device — no domain (device_valid=false) ───────────── */

static void test_cpu_rapl_device_constructor()
{
    rapl_stub_reset();
    cpu_rapl_device dev(nullptr, "cpu_core", "test-cpu-0", nullptr);
    PT_ASSERT_TRUE(!dev.device_present());
}

static void test_cpu_rapl_device_power_usage_no_domain()
{
    rapl_stub_reset();
    cpu_rapl_device dev(nullptr);
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_cpu_rapl_device_measurement_no_domain()
{
    rapl_stub_reset();
    cpu_rapl_device dev(nullptr);
    /* device_valid=false → start/end are no-ops, no T records consumed */
    dev.start_measurement();
    dev.end_measurement();
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_cpu_rapl_device_json()
{
    rapl_stub_reset();
    cpu_rapl_device dev(nullptr, "cpu_core", "test-cpu-0", nullptr);
    std::string js = dev.serialize();
    PT_ASSERT_TRUE(js.find("\"device_valid\":false") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_energy\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"consumed_power\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_sec\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_usec\"") != std::string::npos);
}

/* ── cpu_rapl_device — domain present, full cycle ───────────────── */

static void test_cpu_rapl_domain_cycle()
{
    /*
     * Energy sequence: ctor=100J, start=150J, end=175J
     * Time:   ctor=1000.0s, start=1001.0s, end=1003.5s → delta=2.5s
     * Power:  (175 - 150) / 2.5 = 10.0 W
     */
    rapl_stub_reset();
    rapl_pp0_present = true;
    rapl_push_pp0_energy(100.0);   /* ctor: last_energy = 100 */
    rapl_push_pp0_energy(150.0);   /* start: last_energy = 150 */
    rapl_push_pp0_energy(175.0);   /* end: energy = 175 → 25J/2.5s = 10W */

    cpudevice parent("cpu", "test-cpu", nullptr);
    begin_replay(DATA_DIR + "/rapl_domain_cycle.ptrecord");

    cpu_rapl_device dev(&parent, "cpu_core", "test-cpu", nullptr);
    PT_ASSERT_TRUE(dev.device_present());

    dev.start_measurement();
    dev.end_measurement();

    PT_ASSERT_TRUE(fabs(dev.power_usage(nullptr, nullptr) - 10.0) < 0.001);
    reset_replay();
}

static void test_cpu_rapl_zero_delta()
{
    /*
     * start_time == end_time → delta=0 → consumed_power stays 0
     * end_measurement must NOT read energy (queue has only 2 entries).
     */
    rapl_stub_reset();
    rapl_pp0_present = true;
    rapl_push_pp0_energy(100.0);   /* ctor */
    rapl_push_pp0_energy(150.0);   /* start */
    /* no end entry — would assert-fail if end_measurement reads energy */

    cpudevice parent("cpu", "test-cpu", nullptr);
    begin_replay(DATA_DIR + "/rapl_zero_delta.ptrecord");

    cpu_rapl_device dev(&parent, "cpu_core", "test-cpu", nullptr);
    dev.start_measurement();
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
    reset_replay();
}

static void test_cpu_rapl_near_zero_delta()
{
    /*
     * delta = 5us (0.000005s) < 10us threshold → consumed_power stays 0
     */
    rapl_stub_reset();
    rapl_pp0_present = true;
    rapl_push_pp0_energy(100.0);   /* ctor */
    rapl_push_pp0_energy(150.0);   /* start */
    /* no end entry needed if it guards correctly and skips the read */

    cpudevice parent("cpu", "test-cpu", nullptr);
    begin_replay(DATA_DIR + "/rapl_near_zero.ptrecord");

    cpu_rapl_device dev(&parent, "cpu_core", "test-cpu", nullptr);
    dev.start_measurement();
    dev.end_measurement();

    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
    reset_replay();
}

/* ── dram_rapl_device ────────────────────────────────────────────── */

static void test_dram_rapl_device_constructor()
{
    rapl_stub_reset();
    dram_rapl_device dev(nullptr, "dram_core", "test-dram-0", nullptr);
    PT_ASSERT_TRUE(!dev.device_present());
}

static void test_dram_rapl_device_power_usage_no_domain()
{
    rapl_stub_reset();
    dram_rapl_device dev(nullptr);
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_dram_rapl_domain_cycle()
{
    rapl_stub_reset();
    rapl_dram_present = true;
    rapl_push_dram_energy(200.0);
    rapl_push_dram_energy(210.0);   /* start: last_energy = 210 */
    rapl_push_dram_energy(215.0);   /* end: 5J/2.5s = 2.0W */

    cpudevice parent("dram", "test-dram", nullptr);
    begin_replay(DATA_DIR + "/rapl_domain_cycle.ptrecord");

    dram_rapl_device dev(&parent, "dram_core", "test-dram", nullptr);
    PT_ASSERT_TRUE(dev.device_present());

    dev.start_measurement();
    dev.end_measurement();

    PT_ASSERT_TRUE(fabs(dev.power_usage(nullptr, nullptr) - 2.0) < 0.001);
    reset_replay();
}

static void test_dram_rapl_zero_delta()
{
    rapl_stub_reset();
    rapl_dram_present = true;
    rapl_push_dram_energy(200.0);
    rapl_push_dram_energy(210.0);

    cpudevice parent("dram", "test-dram", nullptr);
    begin_replay(DATA_DIR + "/rapl_zero_delta.ptrecord");

    dram_rapl_device dev(&parent, "dram_core", "test-dram", nullptr);
    dev.start_measurement();
    dev.end_measurement();
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
    reset_replay();
}

static void test_dram_rapl_near_zero_delta()
{
    rapl_stub_reset();
    rapl_dram_present = true;
    rapl_push_dram_energy(200.0);
    rapl_push_dram_energy(210.0);
    cpudevice parent("dram", "test-dram", nullptr);
    begin_replay(DATA_DIR + "/rapl_near_zero.ptrecord");
    dram_rapl_device dev(&parent, "dram_core", "test-dram", nullptr);
    dev.start_measurement();
    dev.end_measurement();
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
    reset_replay();
}

static void test_dram_rapl_device_json()
{
    rapl_stub_reset();
    dram_rapl_device dev(nullptr, "dram_core", "test-dram-0", nullptr);
    std::string js = dev.serialize();
    PT_ASSERT_TRUE(js.find("\"device_valid\":false") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_sec\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_usec\"") != std::string::npos);
}

/* ── gpu_rapl_device ─────────────────────────────────────────────── */

static void test_gpu_rapl_device_constructor()
{
    rapl_stub_reset();
    gpu_rapl_device dev(nullptr);
    PT_ASSERT_TRUE(!dev.device_present());
}

static void test_gpu_rapl_device_power_usage_no_domain()
{
    rapl_stub_reset();
    gpu_rapl_device dev(nullptr);
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_gpu_rapl_device_measurement_no_op()
{
    rapl_stub_reset();
    /* device_valid=false → start/end early-return */
    gpu_rapl_device dev(nullptr);
    dev.start_measurement();
    dev.end_measurement();
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_gpu_rapl_domain_cycle()
{
    rapl_stub_reset();
    rapl_pp1_present = true;
    rapl_push_pp1_energy(500.0);
    rapl_push_pp1_energy(520.0);   /* start: last_energy = 520 */
    rapl_push_pp1_energy(545.0);   /* end: 25J/2.5s = 10W */

    i915gpu parent;
    begin_replay(DATA_DIR + "/rapl_domain_cycle.ptrecord");

    gpu_rapl_device dev(&parent);
    PT_ASSERT_TRUE(dev.device_present());

    dev.start_measurement();
    dev.end_measurement();

    PT_ASSERT_TRUE(fabs(dev.power_usage(nullptr, nullptr) - 10.0) < 0.001);
    reset_replay();
}

static void test_gpu_rapl_zero_delta()
{
    rapl_stub_reset();
    rapl_pp1_present = true;
    rapl_push_pp1_energy(500.0);
    rapl_push_pp1_energy(520.0);

    i915gpu parent;
    begin_replay(DATA_DIR + "/rapl_zero_delta.ptrecord");

    gpu_rapl_device dev(&parent);
    dev.start_measurement();
    dev.end_measurement();
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
    reset_replay();
}

static void test_gpu_rapl_near_zero_delta()
{
    rapl_stub_reset();
    rapl_pp1_present = true;
    rapl_push_pp1_energy(500.0);
    rapl_push_pp1_energy(520.0);
    i915gpu parent;
    begin_replay(DATA_DIR + "/rapl_near_zero.ptrecord");
    gpu_rapl_device dev(&parent);
    dev.start_measurement();
    dev.end_measurement();
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
    reset_replay();
}

static void test_gpu_rapl_device_json()
{
    rapl_stub_reset();
    gpu_rapl_device dev(nullptr);
    std::string js = dev.serialize();
    PT_ASSERT_TRUE(js.find("\"device_valid\":false") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_sec\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_usec\"") != std::string::npos);
}

int main()
{
    std::cout << "=== RAPL device tests ===\n";
    PT_RUN_TEST(test_cpu_rapl_device_constructor);
    PT_RUN_TEST(test_cpu_rapl_device_power_usage_no_domain);
    PT_RUN_TEST(test_cpu_rapl_device_measurement_no_domain);
    PT_RUN_TEST(test_cpu_rapl_device_json);
    PT_RUN_TEST(test_cpu_rapl_domain_cycle);
    PT_RUN_TEST(test_cpu_rapl_zero_delta);
    PT_RUN_TEST(test_cpu_rapl_near_zero_delta);
    PT_RUN_TEST(test_dram_rapl_device_constructor);
    PT_RUN_TEST(test_dram_rapl_device_power_usage_no_domain);
    PT_RUN_TEST(test_dram_rapl_domain_cycle);
    PT_RUN_TEST(test_dram_rapl_zero_delta);
    PT_RUN_TEST(test_dram_rapl_near_zero_delta);
    PT_RUN_TEST(test_dram_rapl_device_json);
    PT_RUN_TEST(test_gpu_rapl_device_constructor);
    PT_RUN_TEST(test_gpu_rapl_device_power_usage_no_domain);
    PT_RUN_TEST(test_gpu_rapl_device_measurement_no_op);
    PT_RUN_TEST(test_gpu_rapl_domain_cycle);
    PT_RUN_TEST(test_gpu_rapl_zero_delta);
    PT_RUN_TEST(test_gpu_rapl_near_zero_delta);
    PT_RUN_TEST(test_gpu_rapl_device_json);
    return pt_test_summary();
}


