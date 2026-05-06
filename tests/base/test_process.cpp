/*
 * Snapshot tests for process::serialize()
 *
 * process is the primary power_consumer derived class.
 * The constructor reads two /proc files (via read_file_content):
 *   /proc/{pid}/status  — only when tid==0; reads Tgid line
 *   /proc/{pid}/cmdline — always; empty → kernel thread
 *
 * We pass tid == pid (non-zero) to skip the status read, leaving
 * exactly one read per construction.
 *
 * Tests:
 *  kernel_thread  — cmdline absent (N record) → is_kernel=1
 *  user_process   — cmdline present ("/bin/bash") → is_kernel=0
 */
#include <string>
#include <vector>
#include "process/process.h"
#include "test_framework.h"
#include "../test_helper.h"

extern std::vector<class power_consumer *> all_power;

double measurement_time = 1.0;

static const std::string DATA_DIR = TEST_DATA_DIR;

static void test_kernel_thread()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/process_kernel.ptrecord");

	/* tid == pid (non-zero) → skips /proc/1234/status read */
	process p("kworker/0:0", 1234, 1234);
	test_framework_manager::get().reset();

	std::string got = p.serialize();

	/* power_consumer base fields */
	PT_ASSERT_TRUE(got.find("\"accumulated_runtime\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"wake_ups\":0") != std::string::npos);
	/* process-specific fields */
	PT_ASSERT_TRUE(got.find("\"comm\":\"kworker/0:0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"pid\":1234") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"tgid\":1234") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"is_kernel\":1") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"running\":0") != std::string::npos);
}

static void test_user_process()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/process_user.ptrecord");

	process p("bash", 5678, 5678);
	test_framework_manager::get().reset();

	std::string got = p.serialize();

	PT_ASSERT_TRUE(got.find("\"comm\":\"bash\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"pid\":5678") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"is_kernel\":0") != std::string::npos);
	/* desc is built from the cmdline content */
	PT_ASSERT_TRUE(got.find("\"desc\":") != std::string::npos);
}

static void test_schedule_deschedule()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/process_user.ptrecord");

	process p("bash", 5678, 5678);
	test_framework_manager::get().reset();  /* construction done, clear replay state */

	/* schedule at t=1000, deschedule at t=5001000: delta = 5000000 */
	p.schedule_thread(1000, 1);
	p.deschedule_thread(5001000, 1);

	std::string got = p.serialize();

	PT_ASSERT_TRUE(got.find("\"accumulated_runtime\":5000000") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"running\":0") != std::string::npos);

	/* usage_summary = accumulated_runtime / 1000000.0 / measurement_time / 10
	   = 5000000 / 1000000 / 1.0 / 10 = 0.5 */
	double usage = p.usage_summary();
	PT_ASSERT_TRUE(usage > 0.499 && usage < 0.501);
}

static void test_description_direct()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/process_user.ptrecord");

	process p("bash", 5678, 5678);
	test_framework_manager::get().reset();

	std::string d = p.description();
	PT_ASSERT_TRUE(!d.empty());
	PT_ASSERT_TRUE(p.usage_units_summary() == "%");
}

static void test_account_disk_dirty()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/process_user.ptrecord");

	process p("bash", 5678, 5678);
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(p.disk_hits == 0);
	p.account_disk_dirty();
	PT_ASSERT_TRUE(p.disk_hits == 1);
	p.account_disk_dirty();
	PT_ASSERT_TRUE(p.disk_hits == 2);
}

static void test_find_create_process_idempotent()
{
	test_framework_manager::get().reset();
	clear_processes();

	test_framework_manager::get().set_replay(DATA_DIR + "/process_find.ptrecord");
	class process *p1 = find_create_process("bash", 5678);
	test_framework_manager::get().reset();

	class process *p2 = find_create_process("bash", 5678);

	PT_ASSERT_TRUE(p1 != nullptr);
	PT_ASSERT_TRUE(p1 == p2);
	PT_ASSERT_TRUE(all_processes.size() == 1);

	test_framework_manager::get().set_replay(DATA_DIR + "/process_user.ptrecord");
	class process *thread = new process("bash", 5678, 5678);
	test_framework_manager::get().reset();

	p1->accumulated_runtime = 10;
	thread->pid = 5679;
	thread->tgid = 5678;
	thread->accumulated_runtime = 20;
	all_processes.push_back(thread);
	merge_processes();
	PT_ASSERT_TRUE(all_processes.size() == 1);
	PT_ASSERT_TRUE(p1->accumulated_runtime == 30);

	clear_processes();
	PT_ASSERT_TRUE(all_processes.empty());
}

static void test_all_processes_to_all_power()
{
	test_framework_manager::get().reset();
	clear_processes();
	all_power.clear();

	test_framework_manager::get().set_replay(DATA_DIR + "/process_find.ptrecord");
	class process *p = find_create_process("bash", 5678);
	test_framework_manager::get().reset();

	p->accumulated_runtime = 1000000;
	all_processes_to_all_power();
	PT_ASSERT_TRUE(!all_power.empty());

	clear_processes();
	all_power.clear();
}

int main()
{
	std::cout << "=== process serialize tests ===\n";
	PT_RUN_TEST(test_kernel_thread);
	PT_RUN_TEST(test_user_process);
	PT_RUN_TEST(test_schedule_deschedule);
	PT_RUN_TEST(test_description_direct);
	PT_RUN_TEST(test_account_disk_dirty);
	PT_RUN_TEST(test_find_create_process_idempotent);
	PT_RUN_TEST(test_all_processes_to_all_power);
	return pt_test_summary();
}
