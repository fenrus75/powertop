/*
 * Copyright 2024, Intel Corporation
 *
 * This is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <format>
#include "test_framework.h"

test_framework_manager& test_framework_manager::get() {
	static test_framework_manager instance;
	return instance;
}

#ifdef ENABLE_TEST_FRAMEWORK
test_framework_manager::test_framework_manager() : recording(false), replaying(false) {}

test_framework_manager::~test_framework_manager() {
	if (recording) {
		save();
	}
}

void test_framework_manager::set_record(const std::string& filename) {
	recording = true;
	record_filename = filename;
}

void test_framework_manager::set_replay(const std::string& filename) {
	replaying = true;
	replay_filename = filename;
	load();
}

void test_framework_manager::record_read(const std::string& path, const std::string& content) {
	if (!recording) return;
	recorded_reads.push_back({path, content});
}

void test_framework_manager::record_read_fail(const std::string& path) {
	if (!recording) return;
	recorded_reads.push_back({path, "__POWERTOP_FILE_NOT_FOUND__"});
}

std::string test_framework_manager::replay_read(const std::string& path) {
	if (!replaying) return "";
	if (read_sequences[path].empty()) {
		throw test_exception("TEST FAIL: No more recorded content for read: " + path);
	}
	std::string content = read_sequences[path].front();
	read_sequences[path].pop_front();
	if (content == "__POWERTOP_FILE_NOT_FOUND__")
		return "";
	return content;
}

void test_framework_manager::record_write(const std::string& path, const std::string& content) {
	if (!recording) return;
	recorded_writes.push_back({path, content});
}

void test_framework_manager::replay_write(const std::string& path, const std::string& content) {
	if (!replaying) return;
	if (write_sequences[path].empty()) {
		std::cerr << "TEST FAIL: Unexpected write to: " << path << " (nothing recorded)" << std::endl;
		return;
	}
	std::string expected = write_sequences[path].front();
	write_sequences[path].pop_front();
	if (expected != content) {
		std::cerr << "TEST FAIL: Write mismatch for " << path << std::endl;
		std::cerr << "  Expected: " << expected << std::endl;
		std::cerr << "  Actual:   " << content << std::endl;
	}
}

void test_framework_manager::record_msr(int cpu, uint64_t offset, uint64_t value) {
	if (!recording) return;
	recorded_msrs.push_back({cpu, offset, value});
}

int test_framework_manager::replay_msr(int cpu, uint64_t offset, uint64_t *value) {
	if (!replaying) return -1;
	auto key = std::make_pair(cpu, offset);
	if (msr_sequences[key].empty()) {
		throw test_exception(std::format("TEST FAIL: No more recorded content for MSR read: cpu {} offset 0x{:x}", cpu, offset));
	}
	*value = msr_sequences[key].front();
	msr_sequences[key].pop_front();
	return 0;
}

void test_framework_manager::record_time(struct timeval tv) {
	if (!recording) return;
	recorded_times.push_back(tv);
}

void test_framework_manager::replay_time(struct timeval *tv) {
	if (!replaying) return;
	if (time_sequences.empty()) {
		throw test_exception("TEST FAIL: No more recorded content for time read");
	}
	*tv = time_sequences.front();
	time_sequences.pop_front();
}

void test_framework_manager::save() {
	std::ofstream file(record_filename);
	if (!file) {
		std::cerr << "Failed to open record file: " << record_filename << std::endl;
		return;
	}

	for (const auto& p : recorded_reads) {
		if (p.second == "__POWERTOP_FILE_NOT_FOUND__")
			file << "N " << p.first << std::endl;
		else
			file << "R " << p.first << " " << base64_encode(p.second) << std::endl;
	}
	for (const auto& p : recorded_writes) {
		file << "W " << p.first << " " << base64_encode(p.second) << std::endl;
	}
	for (const auto& m : recorded_msrs) {
		file << "M " << std::get<0>(m) << " " << std::hex << std::get<1>(m) << " " << std::get<2>(m) << std::dec << std::endl;
	}
	for (const auto& t : recorded_times) {
		file << "T " << t.tv_sec << " " << t.tv_usec << std::endl;
	}
}

void test_framework_manager::load() {
	std::ifstream file(replay_filename);
	if (!file) {
		std::cerr << "Failed to open replay file: " << replay_filename << std::endl;
		return;
	}

	std::string line;
	while (getline(file, line)) {
		if (line.empty()) continue;

		size_t first_space = line.find(' ');
		if (first_space == std::string::npos) continue;

		char type = line[0];
		std::string rest = line.substr(first_space + 1);

		if (type == 'N') {
			read_sequences[rest].push_back("__POWERTOP_FILE_NOT_FOUND__");
			continue;
		}

		if (type == 'M') {
			std::stringstream msr_ss(rest);
			int cpu;
			uint64_t offset, value;
			msr_ss >> cpu >> std::hex >> offset >> value;
			msr_sequences[std::make_pair(cpu, offset)].push_back(value);
			continue;
		}

		if (type == 'T') {
			std::stringstream time_ss(rest);
			struct timeval tv;
			time_ss >> tv.tv_sec >> tv.tv_usec;
			time_sequences.push_back(tv);
			continue;
		}

		size_t last_space = rest.rfind(' ');
		if (last_space == std::string::npos) continue;

		std::string path = rest.substr(0, last_space);
		std::string b64_content = rest.substr(last_space + 1);

		if (type == 'R') {
			read_sequences[path].push_back(base64_decode(b64_content));
		} else if (type == 'W') {
			write_sequences[path].push_back(base64_decode(b64_content));
		}
	}
}

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string test_framework_manager::base64_encode(const std::string& in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::string test_framework_manager::base64_decode(const std::string& in) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;

    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}
#else
test_framework_manager::test_framework_manager() {}
test_framework_manager::~test_framework_manager() {}
#endif
