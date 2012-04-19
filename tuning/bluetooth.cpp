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
#include <string.h>
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

#include "../lib.h"
#include "bluetooth.h"

bt_tunable::bt_tunable(void) : tunable("", 1.0, "Good", "Bad", "Unknown")
{
	sprintf(desc, _("Bluetooth device interface status"));
	strcpy(toggle_bad, "/usr/sbin/hciconfig hci0 up &> /dev/null &");
	strcpy(toggle_good, "/usr/sbin/hciconfig hci0 down &> /dev/null");
}



/* structure definitions copied from include/net/bluetooth/hci.h from the 2.6.20 kernel */
#define HCIGETDEVINFO   _IOR('H', 211, int)
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

static int previous_bytes = -1;
static time_t last_check_time = 0;
static int last_check_result;

int bt_tunable::good_bad(void)
{
	struct hci_dev_info devinfo;
	FILE *file;
	int fd;
	int thisbytes = 0;
	int ret;
	int result = TUNE_GOOD;

	fd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (fd < 0)
		return TUNE_GOOD;

	memset(&devinfo, 0, sizeof(devinfo));
	strcpy(devinfo.name, "hci0");
	ret = ioctl(fd, HCIGETDEVINFO, (void *) &devinfo);
	if (ret < 0)
		goto out;

	if ( (devinfo.flags & 1) == 0 &&
		access("/sys/module/hci_usb",F_OK)) /* interface down already */
		goto out;

	thisbytes += devinfo.stat.byte_rx;
	thisbytes += devinfo.stat.byte_tx;

	/* device is active... so we need to leave it on */
	if (thisbytes != previous_bytes)
		goto out;


	/* this check is expensive.. only do it once per minute */
	if (time(NULL) - last_check_time > 60) {
		last_check_result = TUNE_BAD;
		/* now, also check for active connections */
		file = popen("/usr/bin/hcitool con 2> /dev/null", "r");
		if (file) {
			char line[2048];
			/* first line is standard header */
			if (fgets(line, 2047, file) == NULL)
				goto out;
			memset(line, 0, 2048);
			if (fgets(line, 2047, file) == NULL) {
				result = last_check_result = TUNE_GOOD;
				pclose(file);
				goto out;
			}

			pclose(file);
			if (strlen(line) > 0) {
				result = last_check_result = TUNE_GOOD;
				goto out;
			}
		}
		last_check_time = time(NULL);
	};

	result = last_check_result;

out:
	previous_bytes = thisbytes;
	close(fd);
	return result;
}

void bt_tunable::toggle(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		system("/usr/sbin/hciconfig hci0 up &> /dev/null &");
		return;
	}
	system("/usr/sbin/hciconfig hci0 down &> /dev/null");
}

const char *bt_tunable::toggle_script(void)
{
	int good;
	good = good_bad();

	if (good == TUNE_GOOD) {
		return toggle_bad;
	}
	return toggle_good;
}


void add_bt_tunable(void)
{
	struct hci_dev_info devinfo;
	class bt_tunable *bt;
	int fd;
	int ret;

	/* first check if /sys/modules/bluetooth exists, if not, don't probe bluetooth because
	   it would trigger an autoload */

//	if (access("/sys/module/bluetooth",F_OK))
//		return;


	/* check if hci0 exists */
	fd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (fd < 0)
		return;

	memset(&devinfo, 0, sizeof(devinfo));
	strcpy(devinfo.name, "hci0");
	ret = ioctl(fd, HCIGETDEVINFO, (void *) &devinfo);
	close(fd);
	if (ret < 0)
		return;


	bt = new class bt_tunable();
	all_tunables.push_back(bt);
}
