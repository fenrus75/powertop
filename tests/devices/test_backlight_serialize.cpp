/*
 * Snapshot tests for backlight::serialize() and measurement methods.
 *
 * Fixture files built with trace_tool.py add (no live recording needed):
 *   backlight_start.ptrecord  — 2 R records: max_brightness=100, actual_brightness=60
 *   backlight_cycle.ptrecord  — 3 R records: max=100, start=60, end=80
 *
 * dpms_screen_on() uses opendir()+read_sysfs_string() against real DRM sysfs,
 * which would conflict with the replay framework. The virtual display_is_on()
 * hook lets test_backlight override it to return a fixed value.
 */
#include <string>
#include "devices/backlight.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;
static const std::string BL_PATH  = "/sys/class/backlight/lcd";

/* Subclass that fakes DPMS: screen always reported as on */
class test_backlight : public backlight {
public:
test_backlight(const std::string &name, const std::string &path)
: backlight(name, path) {}

protected:
int display_is_on(void) override { return 1; }
};

static void test_constructor()
{
test_framework_manager::get().reset();
/* constructor does no I/O — no fixture needed */
test_backlight bl("lcd", BL_PATH);
std::string got = bl.serialize();
test_framework_manager::get().reset();

PT_ASSERT_TRUE(got.find("\"name\":\"backlight:lcd\"") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"sysfs_path\":\"" + BL_PATH + "\"") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"max_level\":0") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"start_level\":0") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"end_level\":0") != std::string::npos);
}

static void test_start_measurement()
{
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/backlight_start.ptrecord");

test_backlight bl("lcd", BL_PATH);
bl.start_measurement();   /* reads max_brightness=100, actual_brightness=60 */
std::string got = bl.serialize();
test_framework_manager::get().reset();

PT_ASSERT_TRUE(got.find("\"max_level\":100") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"start_level\":60") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"end_level\":0") != std::string::npos);
}

static void test_measurement_cycle()
{
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/backlight_cycle.ptrecord");

test_backlight bl("lcd", BL_PATH);
bl.start_measurement();   /* max=100, start=60 */
bl.end_measurement();     /* end=80; display_is_on() returns 1 (overridden) */
std::string got = bl.serialize();
test_framework_manager::get().reset();

PT_ASSERT_TRUE(got.find("\"max_level\":100") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"start_level\":60") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"end_level\":80") != std::string::npos);

/* utilization = 100 * (80 + 60) / 2 / 100 = 70.0 */
double util = bl.utilization();
PT_ASSERT_TRUE(util > 69.9 && util < 70.1);
}

int main()
{
std::cout << "=== backlight serialize tests ===\n";
PT_RUN_TEST(test_constructor);
PT_RUN_TEST(test_start_measurement);
PT_RUN_TEST(test_measurement_cycle);
return pt_test_summary();
}
