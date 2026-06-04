/*
 * Copyright 2018, Intel Corporation
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
 *      Gayatri Kammela <gayatri.kammela@intel.com>
 */

#include <memory>
#include <vector>

#include "wakeup.h"
#include "../lib.h"

std::vector<std::unique_ptr<wakeup>> wakeup_all;

wakeup::wakeup(const std::string &str, double _score, [[maybe_unused]] const std::string &enable, [[maybe_unused]] const std::string &disable)
{
	score = _score;
	desc = str;
}

wakeup::wakeup(void)
{
	score = 0;
	desc = "";
	// All fields are already defaulted in header
}

void wakeup::collect_json_fields(std::string &_js) const
{
	JSON_FIELD(desc);
	JSON_FIELD(score);
	JSON_KV("enabled", wakeup_value() == WAKEUP_ENABLE);
	JSON_FIELD(toggle_enable);
	JSON_FIELD(toggle_disable);
}

