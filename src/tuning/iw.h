#ifndef __IW_H
#define __IW_H

/*
 * This code has been blatently stolen from
 *
 * nl80211 userspace tool
 *
 * Copyright 2007, 2008	Johannes Berg <johannes@sipsolutions.net>
 *
 * and has been stripped down to just the pieces needed.
 */

/*

Copyright (c) 2007, 2008	Johannes Berg
Copyright (c) 2007		Andy Lutomirski
Copyright (c) 2007		Mike Kershaw
Copyright (c) 2008-2009		Luis R. Rodriguez

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#include <stdbool.h>

#define ETH_ALEN 6

#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_LIBNL20
#define nl_sock nl_handle
#endif

struct nl80211_state {
	struct nl_sock *nl_sock;
	struct nl_cache *nl_cache;
	struct genl_family *nl80211;
};

enum command_identify_by {
	CIB_NONE,
	CIB_PHY,
	CIB_NETDEV,
};

enum id_input {
	II_NONE,
	II_NETDEV,
	II_PHY_NAME,
	II_PHY_IDX,
};

int get_wifi_power_saving(const char *iface);
int set_wifi_power_saving(const char *iface, int state);

#endif /* __IW_H */
