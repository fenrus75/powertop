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

if [ $# -lt 2 ]; then
	echo "Usage: csstoh.sh cssfile header.h"
	exit 1
fi

if [ ! -r $1 ]; then
	echo "Can't find file $1"
	exit 1
fi

if ! echo -n >$2; then
	echo "Can't open file $2 for writing."
	exit 1
fi

echo "#ifndef __INCLUDE_GUARD_CCS_H" >> $2
echo "#define __INCLUDE_GUARD_CCS_H" >> $2
echo >> $2
echo "const char css[] = " >> $2

sed -r 's/^(.*)$/\t\"\1\\n\"/' $1 >> $2

echo ";" >> $2
echo "#endif" >> $2
