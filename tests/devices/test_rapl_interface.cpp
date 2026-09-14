#include <string>
#include <iostream>
#include <cmath>

#include "cpu/rapl/rapl_interface.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;

static void begin_replay(const std::string &fixture)
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(fixture);
}

static void reset_replay()
{
	test_framework_manager::get().reset();
}

/* ── constructor: powercap sysfs path detects all domains ─────── */

static void test_rapl_interface_powercap_domains()
{
	begin_replay(DATA_DIR + "/rapl_interface_powercap.ptrecord");

	c_rapl_interface iface("package-0", 0);

	PT_ASSERT_TRUE(iface.pkg_domain_present());
	PT_ASSERT_TRUE(iface.pp0_domain_present());   /* "core" subdomain */
	PT_ASSERT_TRUE(iface.dram_domain_present());  /* "dram" subdomain */
	PT_ASSERT_TRUE(!iface.pp1_domain_present());  /* no "uncore" in fixture */

	reset_replay();
}

/* ── constructor: no match → MSR fallback (all domains absent) ───── */

static void test_rapl_interface_no_match()
{
	begin_replay(DATA_DIR + "/rapl_interface_nomatch.ptrecord");

	/* "unknown-pkg" won't match "package-0"; empty powercap dir triggers
	 * MSR fallback which returns 0 (not > 0) → no domains detected */
	c_rapl_interface iface("unknown-pkg", 0);

	PT_ASSERT_TRUE(!iface.pkg_domain_present());
	PT_ASSERT_TRUE(!iface.pp0_domain_present());
	PT_ASSERT_TRUE(!iface.dram_domain_present());
	PT_ASSERT_TRUE(!iface.pp1_domain_present());

	reset_replay();
}

/* ── constructor: empty dev_name skips powercap, MSR fails (no hw) ─ */

static void test_rapl_interface_empty_dev_name()
{
	/*
	 * Replay the MSR reads instead of touching the real /dev/cpu/0/msr:
	 * when the tests run as root on RAPL capable hardware the real MSRs
	 * are readable and the domains would be detected.  Replayed MSR reads
	 * return 0 (not > 0) → no domains.
	 */
	begin_replay(DATA_DIR + "/rapl_interface_nomatch.ptrecord");

	c_rapl_interface iface("", 0);

	PT_ASSERT_TRUE(!iface.pkg_domain_present());

	reset_replay();
}

/* ── get_pp0_energy_status reads energy_uj via powercap path ──── */

static void test_rapl_interface_energy_read()
{
	begin_replay(DATA_DIR + "/rapl_interface_energy.ptrecord");

	c_rapl_interface iface("package-0", 0);
	PT_ASSERT_TRUE(iface.pp0_domain_present());

	double status = 0.0;
	int ret = iface.get_pp0_energy_status(&status);

	PT_ASSERT_EQ(ret, 0);
	PT_ASSERT_TRUE(fabs(status - 123.456789) < 0.0001);

	reset_replay();
}

/* ── get_pp0_energy_status returns -1 when domain absent ──────── */

static void test_rapl_interface_energy_no_domain()
{
	/* see test_rapl_interface_empty_dev_name(): don't read real MSRs */
	begin_replay(DATA_DIR + "/rapl_interface_nomatch.ptrecord");

	c_rapl_interface iface("", 0);

	double status = 0.0;
	int ret = iface.get_pp0_energy_status(&status);

	PT_ASSERT_EQ(ret, -1);

	reset_replay();
}

int main()
{
	std::cout << "=== rapl_interface tests ===\n";
	PT_RUN_TEST(test_rapl_interface_powercap_domains);
	PT_RUN_TEST(test_rapl_interface_no_match);
	PT_RUN_TEST(test_rapl_interface_empty_dev_name);
	PT_RUN_TEST(test_rapl_interface_energy_read);
	PT_RUN_TEST(test_rapl_interface_energy_no_domain);
	return pt_test_summary();
}
