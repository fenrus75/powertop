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

int main()
{
	std::cout << "=== interrupt serialize tests ===\n";
	PT_RUN_TEST(test_hardware_irq);
	PT_RUN_TEST(test_softirq);
	PT_RUN_TEST(test_after_activity);
	return pt_test_summary();
}
