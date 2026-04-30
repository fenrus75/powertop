#include <string>
#include <cmath>

#include "cpu/cpu_rapl_device.h"
#include "cpu/dram_rapl_device.h"
#include "devices/gpu_rapl_device.h"
#include "test_framework.h"
#include "../test_helper.h"

/*
 * All RAPL domain-present stubs return false → device_valid stays false.
 * Tests cover the no-domain path (hardware-independent, always-runnable).
 */

void (*ui_notify_user)(const std::string &) = nullptr;

/* ── cpu_rapl_device tests ───────────────────────────────────────── */

static void test_cpu_rapl_device_constructor()
{
    /* nullptr parent is safe: add_child only called when pp0_domain_present() */
    cpu_rapl_device dev(nullptr, "cpu_core", "test-cpu-0", nullptr);
    PT_ASSERT_TRUE(!dev.device_present());
}

static void test_cpu_rapl_device_power_usage_no_domain()
{
    cpu_rapl_device dev(nullptr);
    /* pp0_domain_present() == false → returns 0 */
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_cpu_rapl_device_measurement_cycle()
{
    cpu_rapl_device dev(nullptr);
    dev.start_measurement();
    dev.end_measurement();
    /* time delta is 0 in a test, consumed_power stays 0 */
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_cpu_rapl_device_json()
{
    cpu_rapl_device dev(nullptr, "cpu_core", "test-cpu-0", nullptr);
    std::string js = dev.serialize();
    PT_ASSERT_TRUE(js.find("\"device_valid\":false") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_energy\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"consumed_power\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_sec\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_usec\"") != std::string::npos);
}

/* ── dram_rapl_device tests ──────────────────────────────────────── */

static void test_dram_rapl_device_constructor()
{
    dram_rapl_device dev(nullptr, "dram_core", "test-dram-0", nullptr);
    PT_ASSERT_TRUE(!dev.device_present());
}

static void test_dram_rapl_device_power_usage_no_domain()
{
    dram_rapl_device dev(nullptr);
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_dram_rapl_device_measurement_cycle()
{
    dram_rapl_device dev(nullptr);
    dev.start_measurement();
    dev.end_measurement();
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_dram_rapl_device_json()
{
    dram_rapl_device dev(nullptr, "dram_core", "test-dram-0", nullptr);
    std::string js = dev.serialize();
    PT_ASSERT_TRUE(js.find("\"device_valid\":false") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_energy\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"consumed_power\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_sec\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_usec\"") != std::string::npos);
}

/* ── gpu_rapl_device tests ───────────────────────────────────────── */

static void test_gpu_rapl_device_constructor()
{
    /* nullptr parent safe: add_child only called when pp1_domain_present() */
    gpu_rapl_device dev(nullptr);
    PT_ASSERT_TRUE(!dev.device_present());
}

static void test_gpu_rapl_device_power_usage_no_domain()
{
    gpu_rapl_device dev(nullptr);
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_gpu_rapl_device_measurement_no_op()
{
    /* device_valid=false → start/end are early-return no-ops */
    gpu_rapl_device dev(nullptr);
    dev.start_measurement();
    dev.end_measurement();
    PT_ASSERT_TRUE(dev.power_usage(nullptr, nullptr) == 0.0);
}

static void test_gpu_rapl_device_json()
{
    gpu_rapl_device dev(nullptr);
    std::string js = dev.serialize();
    PT_ASSERT_TRUE(js.find("\"device_valid\":false") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_energy\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"consumed_power\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_sec\"") != std::string::npos);
    PT_ASSERT_TRUE(js.find("\"last_time_usec\"") != std::string::npos);
}

int main()
{
    std::cout << "=== RAPL device tests ===\n";
    PT_RUN_TEST(test_cpu_rapl_device_constructor);
    PT_RUN_TEST(test_cpu_rapl_device_power_usage_no_domain);
    PT_RUN_TEST(test_cpu_rapl_device_measurement_cycle);
    PT_RUN_TEST(test_cpu_rapl_device_json);
    PT_RUN_TEST(test_dram_rapl_device_constructor);
    PT_RUN_TEST(test_dram_rapl_device_power_usage_no_domain);
    PT_RUN_TEST(test_dram_rapl_device_measurement_cycle);
    PT_RUN_TEST(test_dram_rapl_device_json);
    PT_RUN_TEST(test_gpu_rapl_device_constructor);
    PT_RUN_TEST(test_gpu_rapl_device_power_usage_no_domain);
    PT_RUN_TEST(test_gpu_rapl_device_measurement_no_op);
    PT_RUN_TEST(test_gpu_rapl_device_json);
    return pt_test_summary();
}

