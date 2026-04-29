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
#include "process/process.h"
#include "test_framework.h"
#include "../test_helper.h"

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

int main()
{
	std::cout << "=== process serialize tests ===\n";
	PT_RUN_TEST(test_kernel_thread);
	PT_RUN_TEST(test_user_process);
	return pt_test_summary();
}
