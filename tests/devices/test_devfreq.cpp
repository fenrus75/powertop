/*
 * Tests for devfreq start_measurement, end_measurement, and
 * fill_freq_utilization / fill_freq_name.
 *
 * Constructor stores dir_name, no I/O.
 * start_measurement: reads T (stamp_before) then R (trans_stat).
 * end_measurement:   reads R (trans_stat) then T (stamp_after).
 *
 * Fixture design (dir_name = "gpufreq", 1-second sample):
 *   devfreq_start.ptrecord: T 1000 0, R trans_stat (5000, 10000 ms)
 *   devfreq_end.ptrecord:   R trans_stat (5100, 10200 ms), T 1001 0
 *
 * Expected after process_time_stamps (sample_time = 1000000 µs):
 *   100 MHz: 1000 * (5100 - 5000) = 100000 µs → 10.0%
 *   200 MHz: 1000 * (10200 - 10000) = 200000 µs → 20.0%
 *   Idle:    1000000 - 300000 = 700000 µs → 70.0%
 */
#include <string>
#include <cstring>
#include "devices/devfreq.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;

static void test_constructor()
{
devfreq d("gpufreq");
PT_ASSERT_TRUE(d.device_name() == "gpufreq");
PT_ASSERT_TRUE(d.class_name()  == "devfreq");
PT_ASSERT_TRUE(d.dstates.empty());
}

static void test_measurement_cycle()
{
devfreq d("gpufreq");

test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/devfreq_start.ptrecord");
d.start_measurement();

test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/devfreq_end.ptrecord");
d.end_measurement();
test_framework_manager::get().reset();

/* Three states: 100 MHz, 200 MHz, Idle */
PT_ASSERT_TRUE(d.dstates.size() == 3);

/* fill_freq_name uses hz_to_human and left-pads to 15 chars */
std::string n0 = d.fill_freq_name(0);
std::string n1 = d.fill_freq_name(1);
std::string n2 = d.fill_freq_name(2);
PT_ASSERT_TRUE(n0.find("100") != std::string::npos);
PT_ASSERT_TRUE(n1.find("200") != std::string::npos);
PT_ASSERT_TRUE(n2.find("Idle") != std::string::npos);

/* fill_freq_utilization returns " XX.X% " format */
std::string u0 = d.fill_freq_utilization(0);
std::string u1 = d.fill_freq_utilization(1);
std::string u2 = d.fill_freq_utilization(2);
PT_ASSERT_TRUE(u0.find("10.0") != std::string::npos);
PT_ASSERT_TRUE(u1.find("20.0") != std::string::npos);
PT_ASSERT_TRUE(u2.find("70.0") != std::string::npos);
}

static void test_serialize()
{
devfreq d("gpufreq");

test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/devfreq_start.ptrecord");
d.start_measurement();

test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/devfreq_end.ptrecord");
d.end_measurement();
test_framework_manager::get().reset();

std::string got = d.serialize();
PT_ASSERT_TRUE(got.find("\"dir_name\":\"gpufreq\"") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"dstates\":[") != std::string::npos);
/* sample_time should be close to 1000000.0 */
PT_ASSERT_TRUE(got.find("\"sample_time\":") != std::string::npos);
}

int main()
{
std::cout << "=== devfreq tests ===\n";
PT_RUN_TEST(test_constructor);
PT_RUN_TEST(test_measurement_cycle);
PT_RUN_TEST(test_serialize);
return pt_test_summary();
}
