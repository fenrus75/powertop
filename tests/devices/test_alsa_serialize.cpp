/*
 * Snapshot test for alsa::serialize()
 *
 * Feeds fixed sysfs data via the replay framework, drives a full
 * start/end measurement cycle, then asserts that serialize() produces
 * a JSON object containing all expected field values.
 *
 * Fixture: data/alsa_hwC0D0.ptrecord
 *   - modelname / vendor_name: not-found (humanname stays as "alsa:hwC0D0")
 *   - start: power_off_acct=1000000  power_on_acct=500000
 *   - end:   power_off_acct=1500000  power_on_acct=600000
 */
#include <string>

#include "devices/alsa.h"
#include "devices/alsa-pcm.h"
#include "test_framework.h"
#include "../test_helper.h"

/* Stub: defined in main.cpp, not linked here */
void (*ui_notify_user)(const std::string &) = nullptr;

static const std::string DATA_DIR = TEST_DATA_DIR;

static void test_alsa_serialize()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/alsa_hwC0D0.ptrecord");

	alsa dev("hwC0D0", "/sys/class/sound/hwC0D0");
	dev.start_measurement();
	dev.end_measurement();

	test_framework_manager::get().reset();

	std::string got = dev.serialize();

	/* Base class fields */
	PT_ASSERT_TRUE(got.find("\"class\":\"alsa\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"name\":\"alsa:hwC0D0\"") != std::string::npos);

	/* alsa-specific fields */
	PT_ASSERT_TRUE(got.find("\"sysfs_path\":\"/sys/class/sound/hwC0D0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"humanname\":\"alsa:hwC0D0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"start_inactive\":1000000") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"start_active\":500000") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_inactive\":1500000") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_active\":600000") != std::string::npos);
}

/*
 * alsa_pcm: state sampled at start/end of the measurement, shown only while open.
 * Fixtures: data/alsa_pcm_{running_to_idle,closed,missing_status}.ptrecord
 */
static void test_alsa_pcm_open_stream()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/alsa_pcm_running_to_idle.ptrecord");

	alsa codec_dev("hwC0D0", "/sys/class/sound/hwC0D0");
	alsa_pcm pcm(&codec_dev, 0, 0, true, 0);
	pcm.start_measurement();
	pcm.end_measurement();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(pcm.show_in_list());
	PT_ASSERT_TRUE(!pcm.show_in_overview());
	PT_ASSERT_TRUE(pcm.util_units().empty());
	PT_ASSERT_TRUE(pcm.parent_device() == &codec_dev);
	PT_ASSERT_TRUE(pcm.human_name() ==
		"PCM hw:0,0 playback (ALC257 Analog): RUNNING -> PREPARED, pipewire [1873]");

	/* an open stream keeps the codec row visible even if devlist hid it */
	codec_dev.add_stream(&pcm);
	codec_dev.hide = true;
	PT_ASSERT_TRUE(codec_dev.show_in_list());
}

static void test_alsa_pcm_closed_stream()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/alsa_pcm_closed.ptrecord");

	alsa codec_dev("hwC0D0", "/sys/class/sound/hwC0D0");
	alsa_pcm pcm(&codec_dev, 0, 0, true, 0);
	codec_dev.add_stream(&pcm);
	pcm.start_measurement();
	pcm.end_measurement();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(!pcm.show_in_list());
	codec_dev.hide = true;
	PT_ASSERT_TRUE(!codec_dev.show_in_list());
}

/* kernel without CONFIG_SND_VERBOSE_PROCFS: no status files, nothing is reported */
static void test_alsa_pcm_no_procfs()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/alsa_pcm_missing_status.ptrecord");

	alsa codec_dev("hwC0D0", "/sys/class/sound/hwC0D0");
	alsa_pcm pcm(&codec_dev, 0, 0, true, 0);
	pcm.start_measurement();
	pcm.end_measurement();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(!pcm.show_in_list());
}

int main()
{
	PT_RUN_TEST(test_alsa_pcm_open_stream);
	PT_RUN_TEST(test_alsa_pcm_closed_stream);
	PT_RUN_TEST(test_alsa_pcm_no_procfs);
	PT_RUN_TEST(test_alsa_serialize);
	return pt_test_summary();
}
