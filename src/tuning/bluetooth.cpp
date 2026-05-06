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

#include "tuning.h"
#include "tunable.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <utility>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <format>

#include "../lib.h"
#include "bluetooth.h"

bt_tunable::bt_tunable(int id, const std::string &name) : tunable("", 1.0, _("Good"), _("Bad"), _("Unknown"))
{
	dev_id = id;
	bt_name = name;
	desc = pt_format(_("Bluetooth device interface status ({})"), bt_name);
	toggle_bad = pt_format(_("Enable Bluetooth ({})"), bt_name);
	toggle_good = pt_format(_("Disable Bluetooth ({})"), bt_name);
	snap_bytes[0] = snap_bytes[1] = -1;
	snap_time[0]  = snap_time[1]  =  0;
}



/* structure definitions copied from include/net/bluetooth/hci.h from the 2.6.20 kernel */
#define HCIGETDEVINFO   _IOR('H', 211, int)
#define HCIDEVUP        _IOW('H', 201, int)
#define HCIDEVDOWN      _IOW('H', 202, int)
#define BTPROTO_HCI     1

#define __u16 uint16_t
#define __u8 uint8_t
#define __u32 uint32_t

typedef struct {
        __u8 b[6];
} __attribute__((packed)) bdaddr_t;

struct hci_dev_stats {
        __u32 err_rx;
        __u32 err_tx;
        __u32 cmd_tx;
        __u32 evt_rx;
        __u32 acl_tx;
        __u32 acl_rx;
        __u32 sco_tx;
        __u32 sco_rx;
        __u32 byte_rx;
        __u32 byte_tx;
};


struct hci_dev_info {
	__u16 dev_id;
	char  name[8];

	bdaddr_t bdaddr;

	__u32 flags;
	__u8  type;

	__u8  features[8];

	__u32 pkt_type;
	__u32 link_policy;
	__u32 link_mode;

	__u16 acl_mtu;
	__u16 acl_pkts;
	__u16 sco_mtu;
	__u16 sco_pkts;

	struct hci_dev_stats stat;
};

/*
 * Two-level byte-counter snapshots.  snap_bytes[0]/snap_time[0] is the
 * most-recent snapshot; snap_bytes[1]/snap_time[1] is the previous one.
 * snap_bytes[N] == -1 means the slot has not been populated yet.
 *
 * Slot 0 is refreshed at most once per minute.  Each refresh promotes the
 * current slot 0 into slot 1 first, so slot 1 is always 1–2 minutes older
 * than the current reading.  We only declare BT idle when the byte count
 * matches the slot-1 value, giving a guaranteed minimum observation window
 * of one full minute and avoiding false positives from momentary quiet.
 */
int bt_tunable::hci_get_dev_info(unsigned int &flags,
                                  unsigned int &byte_rx,
                                  unsigned int &byte_tx)
{
	struct hci_dev_info devinfo;

	int fd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (fd < 0)
		return -1;

	memset(&devinfo, 0, sizeof(devinfo));
	strncpy(devinfo.name, bt_name.c_str(), sizeof(devinfo.name) - 1);
	int ret = ioctl(fd, HCIGETDEVINFO, (void *) &devinfo);
	close(fd);
	if (ret < 0)
		return -1;

	flags   = devinfo.flags;
	byte_rx = devinfo.stat.byte_rx;
	byte_tx = devinfo.stat.byte_tx;
	return 0;
}

void bt_tunable::hci_set_power(bool up)
{
	int fd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (fd < 0)
		return;
	ioctl(fd, up ? HCIDEVUP : HCIDEVDOWN, dev_id);
	close(fd);
}

time_t bt_tunable::current_time()
{
	return time(nullptr);
}

int bt_tunable::good_bad(void)
{
	unsigned int flags, byte_rx, byte_tx;
	int thisbytes;
	time_t now;

	if (hci_get_dev_info(flags, byte_rx, byte_tx) < 0)
		return TUNE_GOOD;

	/* Interface already down — already in a good power state */
	if ((flags & 1) == 0)
		return TUNE_GOOD;

	thisbytes = byte_rx + byte_tx;
	now = current_time();

	/* Initialise slot 0 on first call */
	if (snap_bytes[0] < 0) {
		snap_bytes[0] = thisbytes;
		snap_time[0]  = now;
		return TUNE_GOOD;
	}

	/* Advance snapshots once per minute */
	if (now - snap_time[0] >= 60) {
		snap_bytes[1] = snap_bytes[0];
		snap_time[1]  = snap_time[0];
		snap_bytes[0] = thisbytes;
		snap_time[0]  = now;
	}

	/* Need the older slot populated before we can make a reliable call */
	if (snap_bytes[1] < 0)
		return TUNE_GOOD;

	/* Bytes changed versus the 1–2 minute old reference: BT is active */
	if (thisbytes != snap_bytes[1])
		return TUNE_GOOD;

	/* No byte activity over the observation window: BT is idle */
	return TUNE_BAD;
}

void bt_tunable::toggle(void)
{
	hci_set_power(good_bad() != TUNE_BAD);
}


void add_bt_tunable(void)
{
	/* first check if /sys/modules/bluetooth exists, if not, don't probe bluetooth because
	   it would trigger an autoload */

//	if (access("/sys/module/bluetooth",F_OK))
//		return;

	for (const auto &entry : list_directory("/sys/class/bluetooth/")) {
		if (!entry.starts_with("hci"))
			continue;

		int id = 0;
		try {
			id = std::stoi(entry.substr(3));
		} catch (...) {
			continue;
		}

		auto *bt = new bt_tunable(id, entry);
		unsigned int flags, byte_rx, byte_tx;
		if (bt->hci_get_dev_info(flags, byte_rx, byte_tx) < 0) {
			delete bt;
			continue;
		}

		all_tunables.push_back(bt);
	}
}

void bt_tunable::collect_json_fields(std::string &_js)
{
	tunable::collect_json_fields(_js);
	JSON_FIELD(dev_id);
	JSON_FIELD(bt_name);
	_js += "\"snap_bytes\": [" + std::to_string(snap_bytes[0]) + ", " + std::to_string(snap_bytes[1]) + "],";
}
