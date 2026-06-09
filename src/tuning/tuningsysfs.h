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

#include <vector>
#include <climits>

#include "tunable.h"

class sysfs_tunable : public tunable {
protected:
	std::string sysfs_path;
	std::string target_value;
	std::string bad_value;
public:
	sysfs_tunable(const std::string &str, const std::string &sysfs_path, const std::string &target_content);

	virtual int good_bad(void) override;

	virtual void toggle(void) override;

	void collect_json_fields(std::string &_js) override;

};

/*
 * Numeric variant of sysfs_tunable.  Reads the sysfs file as a double and
 * compares with >= (higher_is_better=true, the default) or <= so that a
 * value that already exceeds the target is still reported as Good.
 */
class numeric_sysfs_tunable : public sysfs_tunable {
	double target_double;
	bool   higher_is_better;
public:
	numeric_sysfs_tunable(const std::string &str, const std::string &sysfs_path,
	                      double target, bool higher_is_better = true);

	int good_bad(void) override;

	void collect_json_fields(std::string &_js) override;
};

extern void add_sysfs_tunable(const std::string &str, const std::string &_sysfs_path, const std::string &_target_content);
extern void add_numeric_sysfs_tunable(const std::string &str, const std::string &_sysfs_path,
                                      double target, bool higher_is_better = true);
extern void add_sata_tunables(void);

