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
#ifndef _INCLUDE_GUARD_USB_TUNE_H
#define _INCLUDE_GUARD_USB_TUNE_H

#include <vector>

#include "tunable.h"

using namespace std;

class usb_tunable : public tunable {
	char usb_path[4096];
public:
	usb_tunable(const char *usb_path, const char *path);

	virtual int good_bad(void);

	virtual void toggle(void);

	virtual const char *toggle_script(void);

};

extern void add_usb_tunables(void);


#endif
