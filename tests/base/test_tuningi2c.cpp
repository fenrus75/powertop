/*
 * Tests for i2c_tunable serialize, good_bad(), and toggle().
 *
 * Constructor reads:
 *   {path}/name   (1 R)
 *   device_has_runtime_pm → {path}/device/power/runtime_suspended_time
 *                         and optionally {path}/device/power/runtime_active_time
 * serialize() → collect_json_fields() → result_string() → good_bad()
 *   → reads {path}/power/control (1 R)
 *
 * Fixtures:
 *   i2c_device_auto.ptrecord — 3 R: name + suspended_time + control="auto"
 *   i2c_device_on.ptrecord   — R+N+R+R: name + no-suspended + active_time + control="on"
 *   i2c_toggle_auto.ptrecord — 1 R: power/control="auto"
 *   i2c_toggle_on.ptrecord   — 1 R: power/control="on"
 */
#include <string>
#include "tuning/tuningi2c.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;
static const std::string I2C_PATH = "/sys/bus/i2c/devices/i2c-1";
static const std::string I2C_CTL  = I2C_PATH + "/power/control";

static void test_device_has_runtime_pm_auto()
{
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/i2c_device_auto.ptrecord");

i2c_tunable t(I2C_PATH, "i2c-1", false);
std::string got = t.serialize();
test_framework_manager::get().reset();

/* has runtime PM → desc uses "Runtime PM for I2C Device" form */
PT_ASSERT_TRUE(got.find("Runtime PM") != std::string::npos);
PT_ASSERT_TRUE(got.find("SMBus I801") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"i2c_path\":\"" + I2C_CTL + "\"") != std::string::npos);
/* power/control == "auto" → result = "Good" */
PT_ASSERT_TRUE(got.find("\"result\":\"Good\"") != std::string::npos);
}

static void test_device_active_time_on()
{
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/i2c_device_on.ptrecord");

i2c_tunable t(I2C_PATH, "i2c-1", false);
std::string got = t.serialize();
test_framework_manager::get().reset();

/* has runtime PM via active_time → still "Runtime PM" desc */
PT_ASSERT_TRUE(got.find("Runtime PM") != std::string::npos);
PT_ASSERT_TRUE(got.find("SMBus I801") != std::string::npos);
/* power/control == "on" → result = "Bad" */
PT_ASSERT_TRUE(got.find("\"result\":\"Bad\"") != std::string::npos);
}

static void test_toggle_from_good()
{
/* Construct with auto fixture (3 R), swap to toggle fixture (1 R = "auto") */
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/i2c_device_auto.ptrecord");
i2c_tunable t(I2C_PATH, "i2c-1", false);

test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/i2c_toggle_auto.ptrecord");
t.toggle();

auto log = test_framework_manager::get().get_write_log();
test_framework_manager::get().reset();

PT_ASSERT_TRUE(!log.empty());
PT_ASSERT_TRUE(log[0].first == I2C_CTL);
PT_ASSERT_TRUE(log[0].second == "on");
}

static void test_toggle_from_bad()
{
/* Construct with on fixture (4 records), swap to toggle fixture (1 R = "on") */
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/i2c_device_on.ptrecord");
i2c_tunable t(I2C_PATH, "i2c-1", false);

test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/i2c_toggle_on.ptrecord");
t.toggle();

auto log = test_framework_manager::get().get_write_log();
test_framework_manager::get().reset();

PT_ASSERT_TRUE(!log.empty());
PT_ASSERT_TRUE(log[0].first == I2C_CTL);
PT_ASSERT_TRUE(log[0].second == "auto");
}

int main()
{
std::cout << "=== i2c_tunable tests ===\n";
PT_RUN_TEST(test_device_has_runtime_pm_auto);
PT_RUN_TEST(test_device_active_time_on);
PT_RUN_TEST(test_toggle_from_good);
PT_RUN_TEST(test_toggle_from_bad);
return pt_test_summary();
}
