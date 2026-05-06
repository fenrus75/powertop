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
#pragma once

#ifndef __cplusplus
#define __unused __attribute__((unused))
#endif

#ifdef __cplusplus

#include <format>
#include <string_view>
#include <vector>

template<typename... Args>
inline std::string pt_format(std::string_view fmt, Args&&... args)
{
	return std::vformat(fmt, std::make_format_args(args...));
}

#include <cstring>
#include <string>

#endif

#include <libintl.h>
#include <stdint.h>
#include <stdlib.h>

/* Include only for Automake builds */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_NLS
#define _(STRING)    gettext(STRING)
#else
#define _(STRING)    (STRING)
#endif

#ifdef __cplusplus

extern int is_turbo(uint64_t freq, uint64_t max, uint64_t maxmo);

extern int get_max_cpu(void);
extern void set_max_cpu(int cpu);

extern double percentage(double F);
extern std::string hz_to_human(unsigned long hz, int digits = 2);


extern std::string kernel_function(uint64_t address);



#include <chrono>
#include <ctime>
#include <sys/time.h>

extern std::string get_time_string(const std::string &fmt, std::chrono::system_clock::time_point tp);

extern void write_sysfs(const std::string &filename, const std::string &value);
extern int read_sysfs(const std::string &filename, bool *ok = nullptr);
extern uint64_t read_sysfs_uint64(const std::string &filename, bool *ok = nullptr);
extern std::string read_sysfs_string(const std::string &filename);
extern std::string read_file_content(const std::string &filename);
extern std::string pt_readlink(const std::string &path);
extern struct timeval pt_gettime(void);

extern std::string format_watts(double W, unsigned int len);

extern std::string pci_id_to_name(uint16_t vendor, uint16_t device);
extern void end_pci_access(void);


extern std::string fmt_prefix(double n);
extern std::string pretty_print(const std::string &str);
extern int equals(double a, double b);

template<size_t N> void pt_strcpy(char (&d)[N], const char *s)
{
	strncpy(d, s, N);
	d[N-1] = '\0';
}

/* ── JSON serialization helpers ────────────────────────────────────────── */

/* Escape a string value for inclusion in a JSON string literal. */
inline std::string pt_json_escape(const std::string &s)
{
	std::string out;
	out.reserve(s.size());
	for (unsigned char c : s) {
		if      (c == '"')  out += "\\\"";
		else if (c == '\\') out += "\\\\";
		else if (c == '\n') out += "\\n";
		else if (c == '\r') out += "\\r";
		else if (c == '\t') out += "\\t";
		else                out += c;
	}
	return out;
}

/* pt_json_kv — one overload per supported value type. */
inline std::string pt_json_kv(const std::string &k, const std::string &v)
{ return "\"" + k + "\":\"" + pt_json_escape(v) + "\""; }

inline std::string pt_json_kv(const std::string &k, const char *v)
{ return pt_json_kv(k, std::string(v ? v : "")); }

inline std::string pt_json_kv(const std::string &k, bool v)
{ return "\"" + k + "\":" + (v ? "true" : "false"); }

inline std::string pt_json_kv(const std::string &k, int v)
{ return "\"" + k + "\":" + std::to_string(v); }

inline std::string pt_json_kv(const std::string &k, unsigned int v)
{ return "\"" + k + "\":" + std::to_string(v); }

inline std::string pt_json_kv(const std::string &k, long v)
{ return "\"" + k + "\":" + std::to_string(v); }

inline std::string pt_json_kv(const std::string &k, unsigned long v)
{ return "\"" + k + "\":" + std::to_string(v); }

inline std::string pt_json_kv(const std::string &k, long long v)
{ return "\"" + k + "\":" + std::to_string(v); }

inline std::string pt_json_kv(const std::string &k, unsigned long long v)
{ return "\"" + k + "\":" + std::to_string(v); }

inline std::string pt_json_kv(const std::string &k, double v)
{ return "\"" + k + "\":" + std::format("{:.6g}", v); }

/*
 * pt_json_array — serializes a vector of pointers whose pointees have a
 * serialize() const method. Returns a JSON array string "[...]".
 */
template<typename T>
inline std::string pt_json_array(const std::vector<T *> &vec)
{
	std::string out = "[";
	for (size_t i = 0; i < vec.size(); ++i) {
		out += vec[i]->serialize();
		if (i + 1 < vec.size())
			out += ",";
	}
	out += "]";
	return out;
}

/*
 * Serialization macros.
 *
 * Usage in serialize() / collect_json_fields():
 *
 *   std::string serialize() const {
 *       JSON_START();
 *       JSON_FIELD(my_member);          // key = "my_member"
 *       JSON_KV("key", some_method());  // explicit key
 *       JSON_ARRAY("items", vec_);      // vector<T*> whose T has serialize()
 *       JSON_END();
 *   }
 *
 * JSON_START() declares the local accumulator string _js.
 * JSON_END() appends a sentinel field "_end":0 (avoids trailing-comma
 * handling) then closes the object and returns it.
 *
 * For class hierarchies, override collect_json_fields(std::string &_js) and
 * call the parent version first.  serialize() in the base calls
 * collect_json_fields() then JSON_END().
 */
#define JSON_START()      std::string _js = "{"
#define JSON_FIELD(x)     _js += pt_json_kv(#x, (x)) + ","
#define JSON_KV(k, v)     _js += pt_json_kv((k), (v)) + ","
#define JSON_ARRAY(k, v)  _js += "\"" k "\":" + pt_json_array(v) + ","
#define JSON_END()        return _js + "\"_end\":0}"

typedef void (*callback)(const std::string&);
extern void process_directory(const std::string &d_name, callback fn);
extern std::vector<std::string> list_directory(const std::string &path);
extern int utf_ok;
extern std::string get_user_input(unsigned sz);
extern int read_msr(int cpu, uint64_t offset, uint64_t *value);
extern int write_msr(int cpu, uint64_t offset, uint64_t value);

extern void align_string(std::string &str, size_t min_sz, size_t max_sz);

extern void ui_notify_user_ncurses(const std::string &msg);
extern void ui_notify_user_console(const std::string &msg);
extern void (*ui_notify_user) (const std::string &msg);

#endif /* __cplusplus */

