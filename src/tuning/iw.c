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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include "nl80211.h"
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include <asm/errno.h>
#include <linux/genetlink.h>
#include "iw.h"


#ifndef HAVE_LIBNL20
/* libnl 2.0 compatibility code */

static inline struct nl_handle *nl_socket_alloc(void)
{
	return nl_handle_alloc();
}

static inline void nl_socket_free(struct nl_sock *h)
{
	nl_handle_destroy(h);
}

static inline int __genl_ctrl_alloc_cache(struct nl_sock *h, struct nl_cache **cache)
{
	struct nl_cache *tmp = genl_ctrl_alloc_cache(h);
	if (!tmp)
		return -ENOMEM;
	*cache = tmp;
	return 0;
}
#define genl_ctrl_alloc_cache __genl_ctrl_alloc_cache
#endif /* HAVE_LIBNL20 */


static int nl80211_init(struct nl80211_state *state)
{
	int err;

	state->nl_sock = nl_socket_alloc();
	if (!state->nl_sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	if (genl_connect(state->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	if (genl_ctrl_alloc_cache(state->nl_sock, &state->nl_cache)) {
		fprintf(stderr, "Failed to allocate generic netlink cache.\n");
		err = -ENOMEM;
		goto out_handle_destroy;
	}

	state->nl80211 = genl_ctrl_search_by_name(state->nl_cache, "nl80211");
	if (!state->nl80211) {
		err = -ENOENT;
		goto out_cache_free;
	}

	return 0;

 out_cache_free:
	nl_cache_free(state->nl_cache);
 out_handle_destroy:
	nl_socket_free(state->nl_sock);
	return err;
}

static void nl80211_cleanup(struct nl80211_state *state)
{
	genl_family_put(state->nl80211);
	nl_cache_free(state->nl_cache);
	nl_socket_free(state->nl_sock);
}

static int enable_power_save;


static int set_power_save(struct nl80211_state *state,
			  struct nl_cb *cb,
			  struct nl_msg *msg)
{
	enum nl80211_ps_state ps_state;

	ps_state = NL80211_PS_DISABLED;
	if (enable_power_save)
		ps_state = NL80211_PS_ENABLED;

	NLA_PUT_U32(msg, NL80211_ATTR_PS_STATE, ps_state);

	return 0;

 nla_put_failure:
	return -ENOBUFS;
}

static int print_power_save_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_PS_STATE])
		return 0;

	switch (nla_get_u32(attrs[NL80211_ATTR_PS_STATE])) {
	case NL80211_PS_ENABLED:
		enable_power_save = 1;
		break;
	case NL80211_PS_DISABLED:
	default:
		enable_power_save = 0;
		break;
	}

	return NL_SKIP;
}

static int get_power_save(struct nl80211_state *state,
				   struct nl_cb *cb,
				   struct nl_msg *msg)
{
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM,
		  print_power_save_handler, NULL);
	return 0;
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static int __handle_cmd(struct nl80211_state *state, const char *iface, int get)
{
	struct nl_cb *cb;
	struct nl_msg *msg;
	int devidx = 0;
	int err;

	devidx = if_nametoindex(iface);
	if (devidx == 0)
		devidx = -1;
	if (devidx < 0)
		return -errno;

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return 2;
	}

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		err = 2;
		goto out_free_msg;
	}

	if (get)
		genlmsg_put(msg, 0, 0, genl_family_get_id(state->nl80211), 0,
			    0, NL80211_CMD_GET_POWER_SAVE, 0);
	else
		genlmsg_put(msg, 0, 0, genl_family_get_id(state->nl80211), 0,
			    0, NL80211_CMD_SET_POWER_SAVE, 0);


	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);

	if (get)
		err = get_power_save(state, cb, msg);
	else
		err = set_power_save(state, cb, msg);

	if (err)
		goto out;

	err = nl_send_auto_complete(state->nl_sock, msg);
	if (err < 0)
		goto out;

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

	while (err > 0)
		nl_recvmsgs(state->nl_sock, cb);
 out:
	nl_cb_put(cb);
 out_free_msg:
	nlmsg_free(msg);
	return err;
 nla_put_failure:
	fprintf(stderr, "building message failed\n");
	return 2;
}


int set_wifi_power_saving(const char *iface, int state)
{
	struct nl80211_state nlstate;
	int err;

	err = nl80211_init(&nlstate);
	if (err)
		return 1;

	enable_power_save = state;
	err = __handle_cmd(&nlstate, iface, 0);

	nl80211_cleanup(&nlstate);

	return err;
}


int get_wifi_power_saving(const char *iface)
{
	struct nl80211_state nlstate;
	int err;

	enable_power_save = 0;

	err = nl80211_init(&nlstate);
	if (err)
		return 1;

	err = __handle_cmd(&nlstate, iface, 1);

	nl80211_cleanup(&nlstate);

	if (err) /* not a wifi interface */
		return 1;

	return enable_power_save;
}
