/*
 * Tests for pure (no-I/O) functions in src/lib.cpp:
 *   is_turbo, percentage, hz_to_human, equals, fmt_prefix,
 *   format_watts, pretty_print, pci_id_to_name (HAVE_NO_PCI stub),
 *   end_pci_access (HAVE_NO_PCI stub)
 *
 * No test-framework fixtures needed — all inputs are computed inline.
 * Live system data informs the frequency values used (cpu0 max = 4500000 kHz).
 */
#include <cstdlib>
#include "lib.h"
#include "test_framework.h"
#include "../test_helper.h"

/* Stub: defined in main.cpp, not linked here */
void (*ui_notify_user)(const std::string &) = nullptr;

/* ── is_turbo ───────────────────────────────────────────────────────────── */

static void test_is_turbo_true()
{
	/* freq == max, and max == maxmo + 1000 */
	PT_ASSERT_EQ(is_turbo(4500000, 4500000, 4499000), 1);
}

static void test_is_turbo_false_freq_not_max()
{
	PT_ASSERT_EQ(is_turbo(4000000, 4500000, 4499000), 0);
}

static void test_is_turbo_false_maxmo_wrong()
{
	/* freq == max but maxmo + 1000 != max */
	PT_ASSERT_EQ(is_turbo(4500000, 4500000, 3000000), 0);
}

static void test_is_turbo_all_zero()
{
	/* 0 + 1000 != 0, so not turbo */
	PT_ASSERT_EQ(is_turbo(0, 0, 0), 0);
}

/* ── percentage ─────────────────────────────────────────────────────────── */

static void test_percentage_half()
{
	PT_ASSERT_EQ(percentage(0.5), 50.0);
}

static void test_percentage_zero()
{
	PT_ASSERT_EQ(percentage(0.0), 0.0);
}

static void test_percentage_negative_clamps_to_zero()
{
	PT_ASSERT_EQ(percentage(-0.5), 0.0);
}

static void test_percentage_above_one_not_clamped()
{
	/* The upper clamp is commented out in the source */
	PT_ASSERT_EQ(percentage(1.5), 150.0);
}

/* ── hz_to_human ────────────────────────────────────────────────────────── */

static void test_hz_to_human_ghz_one_digit()
{
	/* 4500000 kHz (live system max) → "4.5 GHz" */
	PT_ASSERT_EQ(hz_to_human(4500000, 1), "4.5 GHz");
}

static void test_hz_to_human_ghz_two_digits()
{
	PT_ASSERT_EQ(hz_to_human(4500000, 2), "4.50 GHz");
}

static void test_hz_to_human_mhz_range()
{
	/* 800000 kHz = 800 MHz */
	PT_ASSERT_EQ(hz_to_human(800000, 1), "   800 MHz");
}

static void test_hz_to_human_mhz_boundary()
{
	/* 1500 kHz, not > 1500000 → falls to MHz path: (1500+500)/1000 = 2 */
	PT_ASSERT_EQ(hz_to_human(1500, 1), "     2 MHz");
}

static void test_hz_to_human_raw_hz()
{
	/* 500 kHz, not > 1000 → raw integer */
	PT_ASSERT_EQ(hz_to_human(500, 1), "      500");
}

static void test_hz_to_human_zero()
{
	PT_ASSERT_EQ(hz_to_human(0, 1), "        0");
}

/* ── equals ─────────────────────────────────────────────────────────────── */

static void test_equals_identical()
{
	PT_ASSERT_EQ(equals(1.0, 1.0), 1);
}

static void test_equals_different()
{
	PT_ASSERT_EQ(equals(1.0, 2.0), 0);
}

static void test_equals_zero()
{
	PT_ASSERT_EQ(equals(0.0, 0.0), 1);
}

static void test_equals_not_within_epsilon()
{
	PT_ASSERT_EQ(equals(0.1, 0.2), 0);
}

/* ── fmt_prefix ─────────────────────────────────────────────────────────── */

static void test_fmt_prefix_one()
{
	utf_ok = 0;
	PT_ASSERT_EQ(fmt_prefix(1.0), " 1.00 ");
}

static void test_fmt_prefix_kilo()
{
	utf_ok = 0;
	PT_ASSERT_EQ(fmt_prefix(1000.0), " 1.00 k");
}

static void test_fmt_prefix_mega()
{
	utf_ok = 0;
	PT_ASSERT_EQ(fmt_prefix(1e6), " 1.00 M");
}

static void test_fmt_prefix_milli()
{
	utf_ok = 0;
	PT_ASSERT_EQ(fmt_prefix(1e-3), " 1.00 m");
}

static void test_fmt_prefix_micro_ascii()
{
	/* With utf_ok=0, micro prefix is 'u' not 'µ' */
	utf_ok = 0;
	PT_ASSERT_EQ(fmt_prefix(1e-6), " 1.00 u");
}

static void test_fmt_prefix_negative()
{
	utf_ok = 0;
	PT_ASSERT_EQ(fmt_prefix(-1.0), "-1.00 ");
}

static void test_fmt_prefix_zero()
{
	utf_ok = 0;
	PT_ASSERT_EQ(fmt_prefix(0.0), " 0.00 ");
}

/* ── pretty_print ───────────────────────────────────────────────────────── */

static void test_pretty_print_known_key()
{
	PT_ASSERT_EQ(pretty_print("ahci"), "SATA controller");
}

static void test_pretty_print_unknown_key_passthrough()
{
	PT_ASSERT_EQ(pretty_print("some_unknown_device"), "some_unknown_device");
}

/* ── pci_id_to_name (HAVE_NO_PCI stub) ──────────────────────────────────── */

static void test_pci_id_to_name_stub_returns_empty()
{
	PT_ASSERT_EQ(pci_id_to_name(0x8086, 0x1234), std::string(""));
}

/* ── end_pci_access (HAVE_NO_PCI stub) ──────────────────────────────────── */

static void test_end_pci_access_stub()
{
	/* HAVE_NO_PCI stub is a no-op; just confirm it doesn't crash */
	end_pci_access();
}

/* ── hz_to_human: missing MHz digits==2 branch ───────────────────────────── */

static void test_hz_to_human_mhz_two_digits()
{
	/* 800000 kHz = 800 MHz with digits==2 → 4-wide integer format */
	PT_ASSERT_EQ(hz_to_human(800000, 2), " 800 MHz");
}

/* ── fmt_prefix: omag==2 special case (line 414) ────────────────────────── */

static void test_fmt_prefix_hundred()
{
	/* 100.0: scientific "1.00e+02" → omag=2 → hits the omag=-1 reassignment */
	utf_ok = 0;
	std::string r = fmt_prefix(100.0);
	PT_ASSERT_TRUE(r.find("100") != std::string::npos);
}

/* ── fmt_prefix: µ (micro) UTF-8 prefix ─────────────────────────────────── */

static void test_fmt_prefix_micro_utf8()
{
	/* Force UTF-8 mode; µ is the 2-byte sequence 0xc2 0xb5 */
	utf_ok = 1;
	std::string r = fmt_prefix(1e-6);
	PT_ASSERT_TRUE(r.find("µ") != std::string::npos);
	utf_ok = 0; /* restore for subsequent tests */
}

/* ── fmt_prefix: UTF-8 environment detection (lines 382-387) ────────────── */

static void test_fmt_prefix_utf8_detection_ascii()
{
	/* Reset detection state; use non-UTF-8 LANG → utf_ok becomes 0 */
	utf_ok = -1;
	const char *saved = getenv("LANG");
	setenv("LANG", "C", 1);
	std::string r = fmt_prefix(1e-6);
	if (saved) setenv("LANG", saved, 1); else unsetenv("LANG");
	utf_ok = 0; /* restore known state for later tests */
	/* ASCII mode: micro prefix is literal 'u' */
	PT_ASSERT_TRUE(r.find("u") != std::string::npos);
}

static void test_fmt_prefix_utf8_detection_unicode()
{
	/* Reset detection state; use UTF-8 LANG → utf_ok becomes 1 */
	utf_ok = -1;
	const char *saved = getenv("LANG");
	setenv("LANG", "en_US.UTF-8", 1);
	std::string r = fmt_prefix(1e-6);
	if (saved) setenv("LANG", saved, 1); else unsetenv("LANG");
	utf_ok = 0; /* restore known state for later tests */
	/* UTF-8 mode: micro prefix is the µ character */
	PT_ASSERT_TRUE(r.find("µ") != std::string::npos);
}

/* ── format_watts ────────────────────────────────────────────────────────── */

static void test_format_watts_normal()
{
	utf_ok = 0;
	std::string s = format_watts(0.5, 0);
	/* 0.5 W → expressed in mW prefix range; "W" suffix always present */
	PT_ASSERT_TRUE(s.find("W") != std::string::npos);
}

static void test_format_watts_tiny_zero()
{
	utf_ok = 0;
	/* W < 0.0001 → overrides to "    0 mW" fixed string */
	std::string s = format_watts(0.00001, 0);
	PT_ASSERT_TRUE(s.find("0 mW") != std::string::npos);
}

static void test_format_watts_aligned()
{
	utf_ok = 0;
	/* len > current string → align_string pads with spaces */
	std::string s = format_watts(1.0, 20);
	PT_ASSERT_TRUE(s.size() >= 20);
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main()
{
	std::cout << "=== is_turbo tests ===\n";
	PT_RUN_TEST(test_is_turbo_true);
	PT_RUN_TEST(test_is_turbo_false_freq_not_max);
	PT_RUN_TEST(test_is_turbo_false_maxmo_wrong);
	PT_RUN_TEST(test_is_turbo_all_zero);

	std::cout << "\n=== percentage tests ===\n";
	PT_RUN_TEST(test_percentage_half);
	PT_RUN_TEST(test_percentage_zero);
	PT_RUN_TEST(test_percentage_negative_clamps_to_zero);
	PT_RUN_TEST(test_percentage_above_one_not_clamped);

	std::cout << "\n=== hz_to_human tests ===\n";
	PT_RUN_TEST(test_hz_to_human_ghz_one_digit);
	PT_RUN_TEST(test_hz_to_human_ghz_two_digits);
	PT_RUN_TEST(test_hz_to_human_mhz_range);
	PT_RUN_TEST(test_hz_to_human_mhz_boundary);
	PT_RUN_TEST(test_hz_to_human_raw_hz);
	PT_RUN_TEST(test_hz_to_human_zero);

	std::cout << "\n=== equals tests ===\n";
	PT_RUN_TEST(test_equals_identical);
	PT_RUN_TEST(test_equals_different);
	PT_RUN_TEST(test_equals_zero);
	PT_RUN_TEST(test_equals_not_within_epsilon);

	std::cout << "\n=== fmt_prefix tests ===\n";
	PT_RUN_TEST(test_fmt_prefix_one);
	PT_RUN_TEST(test_fmt_prefix_kilo);
	PT_RUN_TEST(test_fmt_prefix_mega);
	PT_RUN_TEST(test_fmt_prefix_milli);
	PT_RUN_TEST(test_fmt_prefix_micro_ascii);
	PT_RUN_TEST(test_fmt_prefix_negative);
	PT_RUN_TEST(test_fmt_prefix_zero);

	std::cout << "\n=== pretty_print tests ===\n";
	PT_RUN_TEST(test_pretty_print_known_key);
	PT_RUN_TEST(test_pretty_print_unknown_key_passthrough);

	std::cout << "\n=== pci_id_to_name tests ===\n";
	PT_RUN_TEST(test_pci_id_to_name_stub_returns_empty);

	std::cout << "\n=== end_pci_access tests ===\n";
	PT_RUN_TEST(test_end_pci_access_stub);

	std::cout << "\n=== hz_to_human extra branch ===\n";
	PT_RUN_TEST(test_hz_to_human_mhz_two_digits);

	std::cout << "\n=== fmt_prefix extra branches ===\n";
	PT_RUN_TEST(test_fmt_prefix_hundred);
	PT_RUN_TEST(test_fmt_prefix_micro_utf8);
	PT_RUN_TEST(test_fmt_prefix_utf8_detection_ascii);
	PT_RUN_TEST(test_fmt_prefix_utf8_detection_unicode);

	std::cout << "\n=== format_watts tests ===\n";
	PT_RUN_TEST(test_format_watts_normal);
	PT_RUN_TEST(test_format_watts_tiny_zero);
	PT_RUN_TEST(test_format_watts_aligned);

	return pt_test_summary();
}
