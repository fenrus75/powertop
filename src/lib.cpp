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

char *hz_to_human(unsigned long hz, char *buffer, int digits)
{
	unsigned long long Hz;

	buffer[0] = 0;

	Hz = hz;

	/* default: just put the Number in */
	sprintf(buffer,"%9lli", Hz);

	if (Hz>1000) {
		if (digits == 2)
			sprintf(buffer, "%4lli MHz", (Hz+500)/1000);
		else
			sprintf(buffer, "%6lli MHz", (Hz+500)/1000);
	}

	if (Hz>1500000) {
		if (digits == 2)
			sprintf(buffer, "%4.2f GHz", (Hz+5000.0)/1000000);
		else
			sprintf(buffer, "%3.1f GHz", (Hz+5000.0)/1000000);
	}

	return buffer;
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
	char content[4096];
	char *c;

	file.open(filename.c_str(), ios::in);
	if (!file)
		return "";
	try
	{
		file.getline(content, 4096);
		file.close();
		c = strchr(content, '\n');
		if (c)
			*c = 0;
	} catch (std::exception &exc) {
		file.close();
		return "";
	}
	return content;
}

string read_sysfs_string(const char *format, const char *param)
{
	ifstream file;
	char content[4096];
	char *c;
	char filename[8192];


	snprintf(filename, 8191, format, param);

	file.open(filename, ios::in);
	if (!file)
		return "";
	try
	{
		file.getline(content, 4096);
		file.close();
		c = strchr(content, '\n');
		if (c)
			*c = 0;
	} catch (std::exception &exc) {
		file.close();
		return "";
	}
	return content;
}


void format_watts(double W, char *buffer, unsigned int len)
{
	buffer[0] = 0;
	char buf[32];

	sprintf(buffer, _("%7sW"), fmt_prefix(W, buf));

	if (W < 0.0001)
		sprintf(buffer, _("    0 mW"));

	while (mbstowcs(NULL,buffer,0) < len)
		strcat(buffer, " ");
}


#ifndef HAVE_NO_PCI
static struct pci_access *pci_access;

char *pci_id_to_name(uint16_t vendor, uint16_t device, char *buffer, int len)
{
	char *ret;

	buffer[0] = 0;

	if (!pci_access) {
		pci_access = pci_alloc();
		pci_init(pci_access);
	}

	ret = pci_lookup_name(pci_access, buffer, len, PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE, vendor, device);

	return ret;
}

void end_pci_access(void)
{
	if (pci_access)
		pci_free_name_list(pci_access);
}

#else

char *pci_id_to_name(uint16_t vendor, uint16_t device, char *buffer, int len)
{
	return NULL;
}

void end_pci_access(void)
{
}

#endif /* HAVE_NO_PCI */

int utf_ok = -1;



/* pretty print numbers while limiting the precision */
char *fmt_prefix(double n, char *buf)
{
	static const char prefixes[] = "yzafpnum kMGTPEZY";
	char tmpbuf[16];
	int omag, npfx;
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
		sprintf(buf, "NaN");
		return buf;
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
}


char *pretty_print(const char *str, char *buf, int len)
{
	const char *p;

	if (!pretty_print_init)
		init_pretty_print();

	p = pretty_prints[str].c_str();

	if (strlen(p) == 0)
		p = str;

	snprintf(buf, len,  "%s", p);

	if (len)
		buf[len - 1] = 0;
	return buf;
}

int equals(double a, double b)
{
	return fabs(a - b) <= std::numeric_limits<double>::epsilon();
}

void process_directory(const char *d_name, callback fn)
{
	struct dirent *entry;
	DIR *dir;
	dir = opendir(d_name);
	if (!dir)
		return;
	while (1) {
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;
		if (strcmp(entry->d_name, "lo")==0)
			continue;

		fn(entry->d_name);
	}

	closedir(dir);
}

int get_user_input(char *buf, unsigned sz)
{
	fflush(stdout);
	echo();
	/* Upon successful completion, these functions return OK. Otherwise, they return ERR. */
	int ret = getnstr(buf, sz);
	noecho();
	fflush(stdout);
	/* to distinguish between getnstr error and empty line */
	return ret || strlen(buf);
}

int read_msr(int cpu, uint64_t offset, uint64_t *value)
{
	ssize_t retval;
	uint64_t msr;
	int fd;
	char msr_path[256];

	fd = sprintf(msr_path, "/dev/cpu/%d/msr", cpu);

	if (access(msr_path, R_OK) != 0){
		fd = sprintf(msr_path, "/dev/msr%d", cpu);

		if (access(msr_path, R_OK) != 0){
			fprintf(stderr,
			 _("Model-specific registers (MSR)\
			 not found (try enabling CONFIG_X86_MSR).\n"));
			return -1;
		}
	}

	fd = open(msr_path, O_RDONLY);
	if (fd < 0)
		return -1;
	retval = pread(fd, &msr, sizeof msr, offset);
	close(fd);
	if (retval != sizeof msr) {
		return -1;
	}
	*value = msr;

	return retval;
}

int write_msr(int cpu, uint64_t offset, uint64_t value)
{
	ssize_t retval;
	int fd;
	char msr_path[256];

	fd = sprintf(msr_path, "/dev/cpu/%d/msr", cpu);

	if (access(msr_path, R_OK) != 0){
		fd = sprintf(msr_path, "/dev/msr%d", cpu);

		if (access(msr_path, R_OK) != 0){
			fprintf(stderr,
			 _("Model-specific registers (MSR)\
			 not found (try enabling CONFIG_X86_MSR).\n"));
			return -1;
		}
	}

	fd = open(msr_path, O_WRONLY);
	if (fd < 0)
		return -1;
	retval = pwrite(fd, &value, sizeof value, offset);
	close(fd);
	if (retval != sizeof value) {
		return -1;
	}

	return retval;
}
