/*
 * Tests for cpudevice constructor, device_name, human_name, serialize.
 *
 * Constructor calls get_param_index / get_result_index (parameters subsystem),
 * stores classname and device_name; no I/O, cpu pointer may be nullptr.
 * serialize() calls device::collect_json_fields + own fields; no I/O.
 */
#include <string>
#include "cpu/cpudevice.h"
#include "test_framework.h"
#include "../test_helper.h"

static void test_constructor_fields()
{
cpudevice d("cpu", "cpu0", nullptr);
PT_ASSERT_TRUE(d.class_name()  == "cpu");
PT_ASSERT_TRUE(d.device_name() == "cpu0");
/* no guilty → human_name == device_name */
PT_ASSERT_TRUE(d.human_name() == "cpu0");
}

static void test_custom_classname()
{
cpudevice d("package", "pkg0", nullptr);
PT_ASSERT_TRUE(d.class_name()  == "package");
PT_ASSERT_TRUE(d.device_name() == "pkg0");
PT_ASSERT_TRUE(d.human_name()  == "pkg0");
}

static void test_serialize()
{
cpudevice d("cpu", "cpu3", nullptr);
std::string got = d.serialize();

PT_ASSERT_TRUE(got.find("\"_class\":\"cpu\"")    != std::string::npos);
PT_ASSERT_TRUE(got.find("\"_cpuname\":\"cpu3\"") != std::string::npos);
/* child_devices array should be present and empty */
PT_ASSERT_TRUE(got.find("\"child_devices\":[]") != std::string::npos);
}

static void test_add_child()
{
cpudevice parent("cpu", "cpu0", nullptr);
cpudevice *child = new cpudevice("core", "core0", nullptr);
parent.add_child(child);

std::string got = parent.serialize();
PT_ASSERT_TRUE(got.find("\"child_devices\":[{") != std::string::npos);
/* child's cpuname should appear inside the array */
PT_ASSERT_TRUE(got.find("core0") != std::string::npos);

delete child;
}

int main()
{
std::cout << "=== cpudevice tests ===\n";
PT_RUN_TEST(test_constructor_fields);
PT_RUN_TEST(test_custom_classname);
PT_RUN_TEST(test_serialize);
PT_RUN_TEST(test_add_child);
return pt_test_summary();
}
