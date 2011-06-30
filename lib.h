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
 */
#ifndef INCLUDE_GUARD_LIB_H
#define INCLUDE_GUARD_LIB_H

#ifndef DISABLE_I18N
#include <libintl.h>
#endif
#include <stdint.h>

#ifndef DISABLE_I18N
#define _(STRING)    gettext(STRING)
#else
#define _(STRING)    (STRING)
#endif

#define POWERTOP_VERSION "1.98 beta 1"
#define POWERTOP_SHORT_VERSION "1.98"


extern int get_max_cpu(void);
extern void set_max_cpu(int cpu);

extern double percentage(double F);
extern char *hz_to_human(unsigned long hz, char *buffer, int digits = 2);


extern const char *kernel_function(uint64_t address);

class stringless  
{  
public:
	bool operator()(const char * const & lhs, const char * const & rhs) const ;
};  




#include <string>
using namespace std;

extern void write_sysfs(const string &filename, const string &value);
extern int read_sysfs(const string &filename);
extern string read_sysfs_string(const string &filename);
extern string read_sysfs_string(const char *format, const char *param);

extern void format_watts(double W, char *buffer, unsigned int len);

extern char *pci_id_to_name(uint16_t vendor, uint16_t device, char *buffer, int len);
extern void end_pci_access(void);


extern char *fmt_prefix(double n, char *buf);
extern char *pretty_print(const char *str, char *buf, int len);
extern int equals(double a, double b);

typedef void (*callback)(const char*);
extern void process_directory(const char *d_name, callback fn);
extern int utf_ok;

#endif

