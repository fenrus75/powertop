/*
 * Copyright 2024, Intel Corporation
 *
 * This is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <tuple>
#include <stdexcept>
#include <sys/time.h>

class test_exception : public std::runtime_error {
public:
	test_exception(const std::string& msg) : std::runtime_error(msg) {}
};

class test_framework_manager {
public:
	static test_framework_manager& get();

#ifdef ENABLE_TEST_FRAMEWORK
	void set_record(const std::string& filename);
	void set_replay(const std::string& filename);

	bool is_recording() const { return recording; }
	bool is_replaying() const { return replaying; }

	void record_read(const std::string& path, const std::string& content);
	void record_read_fail(const std::string& path);
	std::string replay_read(const std::string& path);

	void record_write(const std::string& path, const std::string& content);
	void replay_write(const std::string& path, const std::string& content);

	const std::vector<std::pair<std::string, std::string>>& get_write_log() const;

	void record_msr(int cpu, uint64_t offset, uint64_t value);
	int replay_msr(int cpu, uint64_t offset, uint64_t *value);

	void record_time(struct timeval tv);
	void replay_time(struct timeval *tv);

	void record_readlink(const std::string& path, const std::string& target);
	std::string replay_readlink(const std::string& path);

	void save();
	void load();
	void reset();
#else
	void set_record(const std::string&) {}
	void set_replay(const std::string&) {}
	void reset() {}

	bool is_recording() const { return false; }
	bool is_replaying() const { return false; }

	void record_read(const std::string&, const std::string&) {}
	void record_read_fail(const std::string&) {}
	std::string replay_read(const std::string&) { return ""; }

	void record_write(const std::string&, const std::string&) {}
	void replay_write(const std::string&, const std::string&) {}

	const std::vector<std::pair<std::string, std::string>>& get_write_log() const {
		static const std::vector<std::pair<std::string, std::string>> empty;
		return empty;
	}

	void record_msr(int, uint64_t, uint64_t) {}
	int replay_msr(int, uint64_t, uint64_t *) { return -1; }

	void record_time(struct timeval) {}
	void replay_time(struct timeval *) {}

	void record_readlink(const std::string&, const std::string&) {}
	std::string replay_readlink(const std::string&) { return ""; }

	void save() {}
	void load() {}
#endif

private:
	test_framework_manager();
	~test_framework_manager();

#ifdef ENABLE_TEST_FRAMEWORK
	bool recording = false;
	bool replaying = false;
	std::string record_filename;
	std::string replay_filename;

	std::map<std::string, std::deque<std::string>> read_sequences;
	std::vector<std::pair<std::string, std::string>> recorded_reads;

	std::map<std::string, std::deque<std::string>> write_sequences;
	std::vector<std::pair<std::string, std::string>> recorded_writes;
	std::vector<std::pair<std::string, std::string>> write_log;

	std::map<std::pair<int, uint64_t>, std::deque<uint64_t>> msr_sequences;
	std::vector<std::tuple<int, uint64_t, uint64_t>> recorded_msrs;

	std::deque<struct timeval> time_sequences;
	std::vector<struct timeval> recorded_times;

	std::map<std::string, std::deque<std::string>> link_sequences;
	std::vector<std::pair<std::string, std::string>> recorded_links;

	std::string base64_encode(const std::string& in);
	std::string base64_decode(const std::string& in);
#endif
};

