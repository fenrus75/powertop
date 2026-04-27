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
#include <string.h>
#include <iostream>
#include <utility>
#include <iostream>
#include <fstream>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <format>

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

using namespace std;

map<unsigned long, string> kallsyms;

static void read_kallsyms(void)
{
	ifstream file;
	char line[1024];
	kallsyms_read = 1;

	file.open("/proc/kallsyms", ios::in);

	while (file) {
		char *c = NULL, *c2 = NULL;
		unsigned long address = 0;
		memset(line, 0, 1024);
		file.getline(line, 1024);
		c = strchr(line, ' ');
		if (!c)
			continue;
		*c = 0;
		c2 = c + 1;
		if (*c2) c2++;
		if (*c2) c2++;

		address = strtoull(line, NULL, 16);
		c = strchr(c2, '\t');
		if (c)
			*c = 0;
		if (address != 0)
			kallsyms[address] = c2;
	}
	file.close();
}

const char *kernel_function(uint64_t address)
{
	const char *c;
	if (!kallsyms_read)
		read_kallsyms();

	c = kallsyms[address].c_str();
	if (!c)
		return "";
	return c;
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


void write_sysfs(const string &filename, const string &value)
{
	ofstream file;

	file.open(filename.c_str(), ios::out);
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

int read_sysfs(const string &filename, bool *ok)
{
	ifstream file;
	int i;

	file.open(filename.c_str(), ios::in);
	if (!file) {
		if (ok)
			*ok = false;
		return 0;
	}
	try
	{
		file >> i;
		if (ok)
			*ok = true;
	} catch (std::exception &exc) {
		if (ok)
			*ok = false;
		i = 0;
	}
	file.close();
	return i;
}

string read_sysfs_string(const string &filename)
{
	ifstream file;
	string content;

	file.open(filename.c_str(), ios::in);
	if (!file)
		return "";
	try
	{
		getline(file, content);
		file.close();
		size_t pos = content.find('\n');
		if (pos != string::npos)
			content.erase(pos);
	} catch (std::exception &exc) {
		file.close();
		return "";
	}
	return content;
}

string read_file_content(const string &filename)
{
	ifstream file;
	string content;

	file.open(filename.c_str(), ios::in);
	if (!file)
		return "";
	try
	{
		content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
	} catch (std::exception &exc) {
		file.close();
		return "";
	}
	return content;
}

void align_string(std::string &str, size_t min_sz, size_t max_sz)
{
	size_t sz;
	const char *buffer = str.c_str();

	/** mbsrtowcs() allows NULL dst and zero sz,
	 * comparing to mbstowcs(), which causes undefined
	 * behaviour under given circumstances*/

	/* start with mbsrtowcs() local mbstate_t * and
	 * NULL dst pointer*/
	sz = mbsrtowcs(NULL, &buffer, max_sz, NULL);
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
string fmt_prefix(double n)
{
	static const char prefixes[] = "yzafpnum kMGTPEZY";
	char tmpbuf[16];
	int omag, npfx;
	char buf[32];
	char *p, *q, pfx, *c;
	int i;

	if (utf_ok == -1) {
		char *g;
		g = getenv("LANG");
		if (g && strstr(g, "UTF-8"))
			utf_ok = 1;
		else
			utf_ok = 0;
	}

	p = buf;

	*p = ' ';
	if (n < 0.0) {
		*p = '-';
		n = -n;
		p++;
	}

	snprintf(tmpbuf, sizeof tmpbuf, "%.2e", n);
	c = strchr(tmpbuf, 'e');
	if (!c) {
		return "NaN";
	}
	omag = atoi(c + 1);

	npfx = ((omag + 27) / 3) - (27/3);
	omag = (omag + 27) % 3;

	q = tmpbuf;
	if (omag == 2)
		omag = -1;

	for (i = 0; i < 3; i++) {
		while (!isdigit(*q))
			q++;
		*p++ = *q++;
		if (i == omag)
			*p++ = '.';
	}
	*p++ = ' ';

	pfx = prefixes[npfx + 8];

	if (pfx == ' ') {
		/* do nothing */
	} else if (pfx == 'u' && utf_ok > 0) {
		strcpy(p, "µ");		/* Mu is a multibyte sequence */
		while (*p)
			p++;
	} else {
		*p++ = pfx;
	}
	*p = '\0';

	return buf;
}

static map<string, string> pretty_prints;
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
		fn(entry->d_name);
	}
	closedir(dir);
}

void process_directory(const std::string &d_name, callback_str fn)
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

	switch (glob(d_glob.c_str(), GLOB_ERR | GLOB_MARK | GLOB_NOSORT, NULL, &g)) {
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
		fn(g.gl_pathv[c]);
	}
	globfree(&g);
}

void process_glob(const std::string &d_glob, callback_str fn)
{
	glob_t g;
	size_t c;

	switch (glob(d_glob.c_str(), GLOB_ERR | GLOB_MARK | GLOB_NOSORT, NULL, &g)) {
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

string get_user_input(unsigned sz)
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

void ui_notify_user_ncurses(const char *frmt, ...)
{
	char notify[UI_NOTIFY_BUFF_SZ];
	va_list list;

	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	attron(COLOR_PAIR(1));
	va_start(list, frmt);
	/* there is no ncurses *print() function which takes
	 * int x, int y and va_list, this is why we use temp
	 * buffer */
	vsnprintf(notify, UI_NOTIFY_BUFF_SZ - 1, frmt, list);
	va_end(list);
	mvprintw(1, 0, "%s", notify);
	attroff(COLOR_PAIR(1));
}

void ui_notify_user_console(const char *frmt, ...)
{
	va_list list;

	va_start(list, frmt);
	vprintf(frmt, list);
	va_end(list);
}
