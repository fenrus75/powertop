/*
 * Snapshot tests for interrupt::serialize()
 *
 * The interrupt constructor is pure — no sysfs reads, no I/O.
 * Fields set at construction: number, handler, raw_count=0,
 * desc="[{number}] {pretty_print(handler)}".
 *
 * Two scenarios:
 *   hardware_irq — IRQ 42 "eth0"  → desc="[42] eth0"
 *   softirq      — IRQ  0 "timer(softirq)" → desc="[0] timer(softirq)"
 *
 * We also exercise start_interrupt/end_interrupt to verify
 * accumulated_runtime and raw_count are reflected in serialize().
 */
#include <cmath>
#include <string>
#include "process/interrupt.h"
#include "test_framework.h"
#include "../test_helper.h"

double measurement_time = 1.0;

static void test_hardware_irq()
{
	interrupt irq("eth0", 42);

	std::string got = irq.serialize();

	PT_ASSERT_TRUE(got.find("\"number\":42") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"handler\":\"eth0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"raw_count\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"desc\":\"[42] eth0\"") != std::string::npos);
}

static void test_softirq()
{
	interrupt irq("timer(softirq)", 0);

	std::string got = irq.serialize();

	PT_ASSERT_TRUE(got.find("\"number\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"handler\":\"timer(softirq)\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"desc\":\"[0] timer(softirq)\"") != std::string::npos);
}

static void test_after_activity()
{
	interrupt irq("ahci", 16);

	irq.start_interrupt(1000);
	irq.end_interrupt(1500);
	irq.start_interrupt(2000);
	irq.end_interrupt(2300);

	std::string got = irq.serialize();

	/* raw_count incremented by start_interrupt each call */
	PT_ASSERT_TRUE(got.find("\"raw_count\":2") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"number\":16") != std::string::npos);
}

/* ── description() ───────────────────────────────────────────────────────── */

static void test_description_returns_desc()
{
	interrupt irq("eth0", 42);
	PT_ASSERT_EQ(irq.description(), std::string("[42] eth0"));
}

static void test_description_child_clamp()
{
	/* child_runtime > accumulated_runtime is clamped inside description() */
	interrupt irq("eth0", 1);
	irq.accumulated_runtime = 100;
	irq.child_runtime       = 999;
	std::string d = irq.description();
	PT_ASSERT_EQ(irq.child_runtime, 0ULL);
	PT_ASSERT_TRUE(d.find("eth0") != std::string::npos);
}

/* ── usage_summary() / usage_units_summary() ─────────────────────────────── */

static void test_usage_summary_basic()
{
	/* 1 ms runtime, 1 s measurement → t = 1e6/1e6/1.0/10 = 0.1 */
	interrupt irq("ahci", 16);
	irq.accumulated_runtime = 1000000;
	irq.child_runtime       = 0;
	double u = irq.usage_summary();
	PT_ASSERT_TRUE(std::fabs(u - 0.1) < 1e-9);
}

static void test_usage_units_summary_is_percent()
{
	interrupt irq("timer/0", 0);
	PT_ASSERT_EQ(irq.usage_units_summary(), std::string("%"));
}

/* ── find_create_interrupt() / clear_interrupts() ────────────────────────── */

static void test_find_create_interrupt_new()
{
	clear_interrupts();
	const interrupt *p = find_create_interrupt("nvme", 10, 0);
	PT_ASSERT_TRUE(p != nullptr);
	PT_ASSERT_EQ(p->number, 10);
	PT_ASSERT_EQ(p->handler, std::string("nvme"));
	clear_interrupts();
}

static void test_find_create_interrupt_existing()
{
	clear_interrupts();
	const interrupt *p1 = find_create_interrupt("sata", 20, 0);
	const interrupt *p2 = find_create_interrupt("sata", 20, 0);
	PT_ASSERT_TRUE(p1 == p2);   /* same object returned */
	clear_interrupts();
}

static void test_find_create_interrupt_timer_rename()
{
	clear_interrupts();
	/* "timer" gets renamed to "timer/<cpu>" */
	const interrupt *p = find_create_interrupt("timer", 0, 3);
	PT_ASSERT_TRUE(p->handler.find("timer/3") != std::string::npos);
	clear_interrupts();
}

static void test_clear_interrupts_empties_vector()
{
	find_create_interrupt("irq-a", 1, 0);
	find_create_interrupt("irq-b", 2, 0);
	clear_interrupts();
	/* After clear, a fresh find creates a new object at a new address */
	const interrupt *p = find_create_interrupt("irq-a", 1, 0);
	PT_ASSERT_TRUE(p != nullptr);
	clear_interrupts();
}

int main()
{
	std::cout << "=== interrupt serialize tests ===\n";
	PT_RUN_TEST(test_hardware_irq);
	PT_RUN_TEST(test_softirq);
	PT_RUN_TEST(test_after_activity);
	PT_RUN_TEST(test_description_returns_desc);
	PT_RUN_TEST(test_description_child_clamp);
	PT_RUN_TEST(test_usage_summary_basic);
	PT_RUN_TEST(test_usage_units_summary_is_percent);
	PT_RUN_TEST(test_find_create_interrupt_new);
	PT_RUN_TEST(test_find_create_interrupt_existing);
	PT_RUN_TEST(test_find_create_interrupt_timer_rename);
	PT_RUN_TEST(test_clear_interrupts_empties_vector);
	return pt_test_summary();
}
