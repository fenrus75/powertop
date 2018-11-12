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
#ifndef _INCLUDE_GUARD_USB_WAKEUP_H
#define _INCLUDE_GUARD_USB_WAKEUP_H

#include <vector>

#include "wakeup.h"

using namespace std;

class usb_wakeup : public wakeup {
	char usb_path[PATH_MAX];
public:
	char interf[4096];
	usb_wakeup(const char *usb_path, const char *iface);

	virtual int wakeup_value(void);

	virtual void wakeup_toggle(void);

	virtual const char *wakeup_toggle_script(void);

};

extern void add_usb_wakeup(void);

#endif
