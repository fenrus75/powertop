/*
 * Unit tests for processdevice.cpp (device_consumer, add_device, all_devices_to_all_power,
 * clear_proc_devices).
 *
 * Uses a minimal mock device subclass — no sysfs I/O needed.
 */

#include <iostream>
#include <cmath>
#include "../test_helper.h"
#include "test_framework.h"
#include "process/processdevice.h"
#include "devices/device.h"
#include "process/powerconsumer.h"

/* Globals defined in stubs */
extern std::vector<class power_consumer *> all_power;
extern std::vector<class device *> all_devices;

double measurement_time = 1.0;

/* ── Minimal mock device ───────────────────────────────────────────── */

class mock_device : public device {
public:
	std::string _name;
	double      _power;
	int         _prio;
	bool        _visible;

	mock_device(const std::string &name,
	            const std::string &path,
	            double power,
	            int    prio,
	            bool   visible = true)
	    : _name(name), _power(power), _prio(prio), _visible(visible)
	{
		real_path = path;          /* device::real_path is public */
	}

	std::string device_name(void)    override { return _name; }
	std::string human_name(void)     override { return _name; }
	double power_usage([[maybe_unused]] struct result_bundle *r,
	                   [[maybe_unused]] struct parameter_bundle *p) override
	{
		return _power;
	}
	int  grouping_prio(void)  override { return _prio; }
	bool show_in_list(void)   override { return _visible; }
};

/* ── Fixture helpers ───────────────────────────────────────────────── */

static void reset_globals()
{
	/* clear_proc_devices handles deletion; all_power holds raw pointers
	 * shared between all_proc_devices and all_power so only delete via
	 * the former.  all_power itself is just a view here. */
	clear_proc_devices();
	all_power.clear();
	all_devices.clear();
}

/* ── Tests ─────────────────────────────────────────────────────────── */

static void test_empty_all_devices(void)
{
	reset_globals();
	all_devices_to_all_power();
	PT_ASSERT_TRUE(all_proc_devices.empty());
	PT_ASSERT_TRUE(all_power.empty());
}

static void test_single_device_added(void)
{
	reset_globals();
	mock_device dev("disk0", "/dev/sda", 1.5, 0);
	all_devices.push_back(&dev);

	all_devices_to_all_power();

	PT_ASSERT_EQ(all_proc_devices.size(), (size_t)1);
	PT_ASSERT_EQ(all_power.size(), (size_t)1);
	PT_ASSERT_TRUE(std::fabs(all_proc_devices[0]->power - 1.5) < 1e-9);
	reset_globals();
}

static void test_invisible_device_not_added(void)
{
	reset_globals();
	mock_device dev("hidden", "/dev/sdb", 2.0, 0, false /* invisible */);
	all_devices.push_back(&dev);

	all_devices_to_all_power();

	PT_ASSERT_TRUE(all_proc_devices.empty());
	reset_globals();
}

static void test_deduplication_aggregates_power(void)
{
	reset_globals();
	/* Two devices sharing the same real_path → power should be summed */
	mock_device d1("net0",       "/sys/net/eth0", 1.0, 0);
	mock_device d2("net0-alias", "/sys/net/eth0", 0.5, 0);
	all_devices.push_back(&d1);
	all_devices.push_back(&d2);

	all_devices_to_all_power();

	PT_ASSERT_EQ(all_proc_devices.size(), (size_t)1);
	/* Power = initial 1.0 from ctor + 0.5 from dedup path */
	PT_ASSERT_TRUE(std::fabs(all_proc_devices[0]->power - 1.5) < 1e-9);
	reset_globals();
}

static void test_deduplication_prio_update(void)
{
	reset_globals();
	/* d2 has higher prio → device pointer should switch to d2 */
	mock_device d1("net0",    "/sys/net/eth0", 1.0, 0);
	mock_device d2("net0-hi", "/sys/net/eth0", 0.5, 5);
	all_devices.push_back(&d1);
	all_devices.push_back(&d2);

	all_devices_to_all_power();

	PT_ASSERT_EQ(all_proc_devices.size(), (size_t)1);
	PT_ASSERT_EQ(all_proc_devices[0]->prio, 5);
	PT_ASSERT_EQ(all_proc_devices[0]->device->device_name(), std::string("net0-hi"));
	reset_globals();
}

static void test_deduplication_prio_no_update(void)
{
	reset_globals();
	/* d2 has lower prio → device pointer should stay with d1 */
	mock_device d1("net0",    "/sys/net/eth0", 1.0, 5);
	mock_device d2("net0-lo", "/sys/net/eth0", 0.5, 0);
	all_devices.push_back(&d1);
	all_devices.push_back(&d2);

	all_devices_to_all_power();

	PT_ASSERT_EQ(all_proc_devices.size(), (size_t)1);
	PT_ASSERT_EQ(all_proc_devices[0]->prio, 5);
	PT_ASSERT_EQ(all_proc_devices[0]->device->device_name(), std::string("net0"));
	reset_globals();
}

static void test_empty_real_path_not_deduplicated(void)
{
	reset_globals();
	/* Devices with empty real_path must not be merged even if both empty */
	mock_device d1("anon1", "", 1.0, 0);
	mock_device d2("anon2", "", 0.5, 0);
	all_devices.push_back(&d1);
	all_devices.push_back(&d2);

	all_devices_to_all_power();

	PT_ASSERT_EQ(all_proc_devices.size(), (size_t)2);
	reset_globals();
}

static void test_clear_proc_devices(void)
{
	reset_globals();
	mock_device dev("gpu", "/sys/gpu/0", 5.0, 0);
	all_devices.push_back(&dev);

	all_devices_to_all_power();
	PT_ASSERT_EQ(all_proc_devices.size(), (size_t)1);

	clear_proc_devices();
	PT_ASSERT_TRUE(all_proc_devices.empty());

	all_power.clear();
	all_devices.clear();
}

static void test_description_returns_human_name(void)
{
	reset_globals();
	mock_device dev("MyDisk", "/dev/nvme0", 3.0, 0);
	all_devices.push_back(&dev);

	all_devices_to_all_power();

	PT_ASSERT_EQ(all_proc_devices[0]->description(), std::string("MyDisk"));
	reset_globals();
}

static void test_witts_returns_power(void)
{
	reset_globals();
	mock_device dev("gpu", "/sys/gpu/1", 7.25, 0);
	all_devices.push_back(&dev);

	all_devices_to_all_power();

	PT_ASSERT_TRUE(std::fabs(all_proc_devices[0]->Witts() - 7.25) < 1e-9);
	reset_globals();
}

static void test_multiple_independent_devices(void)
{
	reset_globals();
	mock_device d1("sda", "/dev/sda", 1.0, 0);
	mock_device d2("sdb", "/dev/sdb", 2.0, 0);
	mock_device d3("sdc", "/dev/sdc", 3.0, 0);
	all_devices.push_back(&d1);
	all_devices.push_back(&d2);
	all_devices.push_back(&d3);

	all_devices_to_all_power();

	PT_ASSERT_EQ(all_proc_devices.size(), (size_t)3);
	PT_ASSERT_EQ(all_power.size(), (size_t)3);
	reset_globals();
}

int main(void)
{
	std::cout << "=== processdevice tests ===\n";
	PT_RUN_TEST(test_empty_all_devices);
	PT_RUN_TEST(test_single_device_added);
	PT_RUN_TEST(test_invisible_device_not_added);
	PT_RUN_TEST(test_deduplication_aggregates_power);
	PT_RUN_TEST(test_deduplication_prio_update);
	PT_RUN_TEST(test_deduplication_prio_no_update);
	PT_RUN_TEST(test_empty_real_path_not_deduplicated);
	PT_RUN_TEST(test_clear_proc_devices);
	PT_RUN_TEST(test_description_returns_human_name);
	PT_RUN_TEST(test_witts_returns_power);
	PT_RUN_TEST(test_multiple_independent_devices);
	return pt_test_summary();
}
