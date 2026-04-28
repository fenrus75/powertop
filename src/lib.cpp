/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 *	Peter Anvin
 */
#include <map>
#include <vector>
#include <cstring>
#include <iostream>
#include <utility>
#include <fstream>
#include <sstream>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <format>

#include "test_framework.h"

#include "lib.h"

#ifndef HAVE_NO_PCI
extern "C" {
#include <pci/pci.h>
}
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <locale.h>
#include <libintl.h>
#include <limits>
#include <math.h>
#include <ncurses.h>
#include <fcntl.h>
#include <glob.h>

static int kallsyms_read = 0;

int is_turbo(uint64_t freq, uint64_t max, uint64_t maxmo)
{
	if (freq != max)
		return 0;
	if (maxmo + 1000 != max)
		return 0;
	return 1;
}

double percentage(double F)
{
	F = F * 100.0;
//	if (F > 100.0)
//		F = 100.0;
	if (F < 0.0)
		F = 0.0;
	return F;
}

std::string hz_to_human(unsigned long hz, int digits)
{
	unsigned long long Hz = hz;

	if (Hz > 1500000) {
		if (digits == 2)
			return std::format("{:4.2f} GHz", (Hz + 5000.0) / 1000000.0);
		else
			return std::format("{:3.1f} GHz", (Hz + 5000.0) / 1000000.0);
	}

	if (Hz > 1000) {
		if (digits == 2)
			return std::format("{:4d} MHz", (int)((Hz + 500) / 1000));
		else
			return std::format("{:6d} MHz", (int)((Hz + 500) / 1000));
	}

	return std::format("{:9d}", (int)Hz);
}

std::map<unsigned long, std::string> kallsyms;

static void read_kallsyms(void)
{
	std::string content = read_file_content("/proc/kallsyms");
	kallsyms_read = 1;

	if (content.empty())
		return;

	std::istringstream stream(content);
	std::string line;

	while (std::getline(stream, line)) {
		size_t pos = line.find(' ');
		if (pos == std::string::npos)
			continue;

		unsigned long address = 0;
		try {
			address = std::stoull(line.substr(0, pos), nullptr, 16);
		} catch (...) {}

		if (address == 0)
			continue;

		std::string rest = line.substr(pos + 1);
		/* skip type character and spaces */
		size_t pos2 = rest.find_first_not_of(" \t");
		if (pos2 != std::string::npos) {
			/* skip the type char (e.g. 'T') and next spaces */
			size_t pos3 = rest.find_first_not_of(" \t", pos2 + 1);
			if (pos3 != std::string::npos) {
				kallsyms[address] = rest.substr(pos3);
			}
		}
	}
}

std::string kernel_function(uint64_t address)
{
	if (!kallsyms_read)
		read_kallsyms();

	if (kallsyms.count(address))
		return kallsyms[address];

	return "";
}

static int _max_cpu;
int get_max_cpu(void)
{
	return _max_cpu;
}

void set_max_cpu(int cpu)
{
	if (cpu > _max_cpu)
		_max_cpu = cpu;
}


void write_sysfs(const std::string &filename, const std::string &value)
{
	if (test_framework_manager::get().is_replaying()) {
		test_framework_manager::get().replay_write(filename, value);
		return;
	}
	if (test_framework_manager::get().is_recording()) {
		test_framework_manager::get().record_write(filename, value);
	}
	std::ofstream file;

	file.open(filename.c_str(), std::ios::out);
	if (!file)
		return;
	try
	{
		file << value;
		file.close();
	} catch (std::exception &exc) {
		return;
	}
}

int read_sysfs(const std::string &filename, bool *ok)
{
	std::string content = read_file_content(filename);
	if (content.empty()) {
		if (ok)
			*ok = false;
		return 0;
	}
	try
	{
		int i = std::stoi(content);
		if (ok)
			*ok = true;
		return i;
	} catch (...) {
		if (ok)
			*ok = false;
		return 0;
	}
}

std::string read_sysfs_string(const std::string &filename)
{
	std::string content = read_file_content(filename);
	size_t pos = content.find('\n');
	if (pos != std::string::npos)
		content.erase(pos);
	return content;
}

std::string read_file_content(const std::string &filename)
{
	if (test_framework_manager::get().is_replaying()) {
		return test_framework_manager::get().replay_read(filename);
	}
	std::ifstream file;
	std::string content;

	file.open(filename.c_str(), std::ios::in);
	if (!file) {
		if (test_framework_manager::get().is_recording()) {
			test_framework_manager::get().record_read_fail(filename);
		}
		return "";
	}
	try
	{
		content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
	} catch (std::exception &exc) {
		file.close();
		if (test_framework_manager::get().is_recording()) {
			test_framework_manager::get().record_read_fail(filename);
		}
		return "";
	}
	if (test_framework_manager::get().is_recording()) {
		test_framework_manager::get().record_read(filename, content);
	}
	return content;
}

struct timeval pt_gettime(void)
{
	struct timeval tv;
	if (test_framework_manager::get().is_replaying()) {
		test_framework_manager::get().replay_time(&tv);
		return tv;
	}
	gettimeofday(&tv, nullptr);
	if (test_framework_manager::get().is_recording()) {
		test_framework_manager::get().record_time(tv);
	}
	return tv;
}

void align_string(std::string &str, size_t min_sz, size_t max_sz)
{
	size_t sz;
	const char *buffer = str.c_str();

	/** mbsrtowcs() allows nullptr dst and zero sz,
	 * comparing to mbstowcs(), which causes undefined
	 * behaviour under given circumstances*/

	/* start with mbsrtowcs() local mbstate_t * and
	 * nullptr dst pointer*/
	sz = mbsrtowcs(nullptr, &buffer, max_sz, nullptr);
	if (sz == (size_t)-1) {
		return;
	}
	while (sz < min_sz) {
		str += " ";
		sz++;
	}
}

std::string format_watts(double W, unsigned int len)
{
	std::string buffer;

	buffer = pt_format(_("{:7s}W"), fmt_prefix(W));

	if (W < 0.0001)
		buffer = _("    0 mW");

	align_string(buffer, len, len);

	return buffer;
}

#ifndef HAVE_NO_PCI
static struct pci_access *pci_access;

std::string pci_id_to_name(uint16_t vendor, uint16_t device)
{
	const int len = 4096;
	std::vector<char> buf(len);

	if (!pci_access) {
		pci_access = pci_alloc();
		pci_init(pci_access);
	}

	char *ret = pci_lookup_name(pci_access, buf.data(), len, PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE, vendor, device);
	return ret ? std::string(ret) : std::string();
}

void end_pci_access(void)
{
	if (pci_access)
		pci_free_name_list(pci_access);
}

#else

std::string pci_id_to_name(uint16_t vendor, uint16_t device)
{
	return std::string();
}

void end_pci_access(void)
{
}

#endif /* HAVE_NO_PCI */

int utf_ok = -1;



/* pretty print numbers while limiting the precision */
std::string fmt_prefix(double n)
{
	static const char prefixes[] = "yzafpnum kMGTPEZY";
	int omag, npfx;
	char pfx;
	std::string res;

	if (utf_ok == -1) {
		char *g;
		g = getenv("LANG");
		if (g && strstr(g, "UTF-8"))
			utf_ok = 1;
		else
			utf_ok = 0;
	}

	if (n < 0.0) {
		res = "-";
		n = -n;
	} else {
		res = " ";
	}

	std::string tmpbuf = std::format("{:.2e}", n);
	size_t e_pos = tmpbuf.find('e');
	if (e_pos == std::string::npos) {
		return "NaN";
	}
	
	try {
		omag = std::stoi(tmpbuf.substr(e_pos + 1));
	} catch (...) {
		return "NaN";
	}

	npfx = ((omag + 27) / 3) - (27/3);
	npfx = std::clamp(npfx, -8, 8);
	omag = (omag + 27) % 3;

	if (omag == 2)
		omag = -1;

	int digits_found = 0;
	for (char c : tmpbuf) {
		if (isdigit(c)) {
			res += c;
			if (digits_found == omag)
				res += '.';
			digits_found++;
		}
		if (digits_found >= 3)
			break;
	}
	res += ' ';

	pfx = prefixes[npfx + 8];

	if (pfx == ' ') {
		/* do nothing */
	} else if (pfx == 'u' && utf_ok > 0) {
		res += "µ";
	} else {
		res += pfx;
	}

	return res;
}

static std::map<std::string, std::string> pretty_prints;
static int pretty_print_init = 0;

static void init_pretty_print(void)
{
	pretty_prints["[12] i8042"] = _("PS/2 Touchpad / Keyboard / Mouse");
	pretty_prints["ahci"] = _("SATA controller");
	pretty_prints["usb-device-8087-0020"] = _("Intel built in USB hub");

	pretty_print_init = 1;
}

std::string pretty_print(const std::string &str)
{
	if (!pretty_print_init)
		init_pretty_print();

	if (pretty_prints.count(str))
		return pretty_prints[str];

	return str;
}

void process_directory(const std::string &d_name, callback fn)
{
	struct dirent *entry;
	DIR *dir;
	dir = opendir(d_name.c_str());
	if (!dir)
		return;
	while (1) {
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;
		fn(std::string(entry->d_name));
	}
	closedir(dir);
}

int equals(double a, double b)
{
	return fabs(a - b) <= std::numeric_limits<double>::epsilon();
}

void process_glob(const std::string &d_glob, callback fn)
{
	glob_t g;
	size_t c;

	switch (glob(d_glob.c_str(), GLOB_ERR | GLOB_MARK | GLOB_NOSORT, nullptr, &g)) {
	case GLOB_NOSPACE:
		fprintf(stderr,_("glob returned GLOB_NOSPACE\n"));
		globfree(&g);
		return;
	case GLOB_ABORTED:
		fprintf(stderr,_("glob returned GLOB_ABORTED\n"));
		globfree(&g);
		return;
	case GLOB_NOMATCH:
		fprintf(stderr,_("glob returned GLOB_NOMATCH\n"));
		globfree(&g);
		return;
	}

	for (c=0; c < g.gl_pathc; c++) {
		fn(std::string(g.gl_pathv[c]));
	}
	globfree(&g);
}

std::string get_user_input(unsigned sz)
{
	std::string buf(sz + 1, '\0');
	fflush(stdout);
	echo();
	/* Upon successful completion, these functions return OK. Otherwise, they return ERR. */
	getnstr(buf.data(), sz);
	noecho();
	fflush(stdout);
	buf.resize(strnlen(buf.data(), sz));
	return buf;
}

int read_msr(int cpu, uint64_t offset, uint64_t *value)
{
	if (test_framework_manager::get().is_replaying()) {
		return test_framework_manager::get().replay_msr(cpu, offset, value);
	}
#if defined(__i386__) || defined(__x86_64__)
	ssize_t retval;
	uint64_t msr;
	int fd;
	std::string msr_path;

	msr_path = std::format("/dev/cpu/{}/msr", cpu);

	if (access(msr_path.c_str(), R_OK) != 0){
		msr_path = std::format("/dev/msr{}", cpu);

		if (access(msr_path.c_str(), R_OK) != 0){
			fprintf(stderr,
			 _("Model-specific registers (MSR)\
			 not found (try enabling CONFIG_X86_MSR).\n"));
			return -1;
		}
	}

	fd = open(msr_path.c_str(), O_RDONLY);
	if (fd < 0)
		return -1;
	retval = pread(fd, &msr, sizeof msr, offset);
	close(fd);
	if (retval != sizeof msr) {
		return -1;
	}
	*value = msr;

	if (test_framework_manager::get().is_recording()) {
		test_framework_manager::get().record_msr(cpu, offset, msr);
	}

	return retval;
#else
	return -1;
#endif
}

int write_msr(int cpu, uint64_t offset, uint64_t value)
{
#if defined(__i386__) || defined(__x86_64__)
	ssize_t retval;
	int fd;
	std::string msr_path;

	msr_path = std::format("/dev/cpu/{}/msr", cpu);

	if (access(msr_path.c_str(), R_OK) != 0){
		msr_path = std::format("/dev/msr{}", cpu);

		if (access(msr_path.c_str(), R_OK) != 0){
			fprintf(stderr,
			 _("Model-specific registers (MSR)\
			 not found (try enabling CONFIG_X86_MSR).\n"));
			return -1;
		}
	}

	fd = open(msr_path.c_str(), O_WRONLY);
	if (fd < 0)
		return -1;
	retval = pwrite(fd, &value, sizeof value, offset);
	close(fd);
	if (retval != sizeof value) {
		return -1;
	}

	return retval;
#else
	return -1;
#endif
}

#define UI_NOTIFY_BUFF_SZ 2048

void ui_notify_user_ncurses(const std::string &msg)
{
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	attron(COLOR_PAIR(1));
	mvprintw(1, 0, "%s", msg.c_str());
	attroff(COLOR_PAIR(1));
}

void ui_notify_user_console(const std::string &msg)
{
	printf("%s\n", msg.c_str());
}
