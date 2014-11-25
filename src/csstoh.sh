#!/bin/sh
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
# or just google for it.
#
# Written by Igor Zhbanov <i.zhbanov at samsung.com>

if [ $# -ne 2 ]; then
	echo "Usage: csstoh.sh cssfile header.h" >&2
	exit 1
fi
if [ ! -f "$1" ]; then
	echo "$1: no such file or directory" >&2
	exit 1
fi
# redirect stdout to a file
exec 1> "$2" || exit $?

# header
cat <<HERE || exit $?
#ifndef __INCLUDE_GUARD_CCS_H
#define __INCLUDE_GUARD_CCS_H

const char css[] =
HERE
# body
sed -r 's/^[ \t]*//; s/^(.*)$/\t\"\1\\n\"/' "$1" || exit $?
# footer
cat <<HERE || exit $?
;
#endif
HERE

# close output file
exec 1>&-
# return status of output file write
exit $?
