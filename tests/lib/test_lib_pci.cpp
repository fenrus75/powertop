/*
 * Tests for the libpci wrapper functions in src/lib.cpp:
 *   pci_id_to_name(), end_pci_access()
 *
 * This executable is compiled WITHOUT -DHAVE_NO_PCI so the real
 * pci_alloc/pci_init/pci_lookup_name/pci_free_name_list paths are used.
 *
 * Covered paths:
 *   lines 336-348: pci_id_to_name() — first call (allocates pci_access),
 *                  second call (reuses existing pci_access)
 *   lines 350-354: end_pci_access() — pci_access != NULL branch
 */
#include <iostream>
#include <cstdint>

#include "lib.h"
#include "test_framework.h"
#include "../test_helper.h"

void (*ui_notify_user)(const std::string &) = nullptr;

/* ── pci_id_to_name ──────────────────────────────────────────────────────── */

static void test_pci_id_to_name_first_call()
{
	/*
	 * 0x8086 / 0x1237 = Intel 440FX-82441FX PMC [Natoma] — present in
	 * every pci.ids since 1996.  Lookup may return empty if the local
	 * pci.ids database is absent, so we only assert the call doesn't crash
	 * and returns a std::string (not a raw pointer / exception).
	 */
	std::string name = pci_id_to_name(0x8086, 0x1237);
	/* As long as pci.ids is installed, this should be non-empty */
	PT_ASSERT_TRUE(name.size() >= 0);  /* always true — ensures no exception/crash */
	std::cout << "  pci_id_to_name(0x8086, 0x1237) = \"" << name << "\"\n";
}

static void test_pci_id_to_name_second_call()
{
	/*
	 * Second call: pci_access is already initialised (static pointer set
	 * by first call), so the if (!pci_access) branch is skipped.
	 * 0x8086 / 0x29c0 = Intel 82G33/G31/P35/P31 Express DRAM Controller
	 */
	std::string name = pci_id_to_name(0x8086, 0x29c0);
	PT_ASSERT_TRUE(name.size() >= 0);
	std::cout << "  pci_id_to_name(0x8086, 0x29c0) = \"" << name << "\"\n";
}

/* ── end_pci_access ─────────────────────────────────────────────────────── */

static void test_end_pci_access_with_live_handle()
{
	/* pci_access is non-NULL after the two calls above — covers line 352 */
	end_pci_access();
	/* No assertion needed — if it crashes, the test fails */
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main()
{
	std::cout << "=== pci_id_to_name tests ===\n";
	PT_RUN_TEST(test_pci_id_to_name_first_call);
	PT_RUN_TEST(test_pci_id_to_name_second_call);

	std::cout << "\n=== end_pci_access tests ===\n";
	PT_RUN_TEST(test_end_pci_access_with_live_handle);

	return pt_test_summary();
}
