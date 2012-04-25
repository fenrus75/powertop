/*
 *	extech - Program for controlling the extech Device
 *	This file is part of PowerTOP
 *
 *      Based on earlier client by Patrick Mochel for Wattsup Pro device
 *	Copyright (c) 2005 Patrick Mochel
 *	Copyright (c) 2006 Intel Corporation
 *	Copyright (c) 2011 Intel Corporation
 *
 *	Authors:
 *	    Patrick Mochel
 *	    Venkatesh Pallipadi
 *	    Arjan van de Ven
 *
 *	Thanks to Rajiv Kapoor for finding out the DTR, RTS line bits issue below
 *	Without that this program would never work.
 *
 *
 *	This program file is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful, but WITHOUT
 *	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *	FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *	for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program in a file named COPYING; if not, write to the
 *	Free Software Foundation, Inc,
 *	51 Franklin Street, Fifth Floor,
 *	Boston, MA 02110-1301 USA
 *	or just google for it.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <sys/stat.h>

#include "measurement.h"
#include "extech.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

struct packet {
	char	buf[256];
	char	op[32];
	double	watts;
	int	len;
};


static int open_device(const char *device_name)
{
	struct stat s;
	int ret;

	ret = stat(device_name, &s);
	if (ret < 0)
		return -1;

	if (!S_ISCHR(s.st_mode))
		return -1;

	ret = access(device_name, R_OK | W_OK);
	if (ret)
		return -1;

	ret = open(device_name, O_RDWR | O_NONBLOCK | O_NOCTTY);
	if (ret < 0)
		return -1;

	return ret;
}


static int setup_serial_device(int dev_fd)
{
	struct termios t;
	int ret;
	int flgs;

	ret = tcgetattr(dev_fd, &t);
	if (ret)
		return ret;

	cfmakeraw(&t);
	cfsetispeed(&t, B9600);
	cfsetospeed(&t, B9600);
	tcflush(dev_fd, TCIFLUSH);

	t.c_iflag = IGNPAR;
	t.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
	t.c_oflag = 0;
	t.c_lflag = 0;
	t.c_cc[VMIN] = 2;
	t.c_cc[VTIME] = 0;

	t.c_iflag &= ~(IXON | IXOFF | IXANY);
	t.c_oflag &= ~(IXON | IXOFF | IXANY);

	ret = tcsetattr(dev_fd, TCSANOW, &t);

	if (ret)
		return ret;

	/*
	 * Root caused by Rajiv Kapoor. Without this extech reads
	 * will fail
	 */

	/* get DTR and RTS line bits settings */
	ioctl(dev_fd, TIOCMGET, &flgs);

	/* set DTR to 1 and RTS to 0 */
	flgs |= TIOCM_DTR;
	flgs &= ~TIOCM_RTS;
	ioctl(dev_fd, TIOCMSET, &flgs);

	return 0;
}


static unsigned int decode_extech_value(unsigned char byt3, unsigned char byt4, char *a)
{
	unsigned int input = ((unsigned int)byt4 << 8) + byt3;
	unsigned int i;
	unsigned int idx;
	unsigned char revnum[] = { 0x0, 0x8, 0x4, 0xc,
				   0x2, 0xa, 0x6, 0xe,
				   0x1, 0x9, 0x5, 0xd,
				   0x3, 0xb, 0x7, 0xf};
	unsigned char revdec[] = { 0x0, 0x2, 0x1, 0x3};

	unsigned int digit_map[] = {0x2, 0x3c, 0x3c0, 0x3c00};
	unsigned int digit_shift[] = {1, 2, 6, 10};

	unsigned int sign;
	unsigned int decimal;

	/* this is basically BCD encoded floating point... but kinda weird */

	decimal = (input & 0xc000) >> 14;
	decimal = revdec[decimal];

	sign = input & 0x1;

	idx = 0;
	if (sign)
		a[idx++] = '+';
	else
		a[idx++] = '-';

	/* first digit is only one or zero */
	a[idx] = '0';
	if ((input & digit_map[0]) >> digit_shift[0])
		a[idx] += 1;

	idx++;
	/* Reverse the remaining three digits and store in the array */
	for (i = 1; i < 4; i++) {
		int dig = ((input & digit_map[i]) >> digit_shift[i]);
		dig = revnum[dig];
		if (dig > 0xa)
			goto error_exit;

		a[idx++] = '0' + dig;
	}

	/* Fit the decimal point where appropriate */
	for (i = 0; i < decimal; i++)
		a[idx - i] = a[idx - i - 1];

	a[idx - decimal] = '.';
	a[++idx] = '0';
	a[++idx] = '\0';

	return 0;
error_exit:
	return -1;
}

static int parse_packet(struct packet * p)
{
	int i;
	int ret;

	p->buf[p->len] = '\0';

	/*
	 * First character in 5 character block should be '02'
	 * Fifth character in 5 character block should be '03'
	 */
	for (i = 0; i < 4; i++) {
		if (p->buf[i * 0] != 2 || p->buf[i * 0 + 4] != 3) {
			printf("Invalid packet\n");
			return -1;
		}
	}

	for (i = 0; i < 1; i++) {
		ret = decode_extech_value(p->buf[5 * i + 2],
					  p->buf[5 * i + 3],
					  &(p->op[8 * i]));
		if (ret) {
			printf("Invalid packet, conversion failed\n");
			return -1;
		}
		p->watts = strtod( &(p->op[8 * i]), NULL);
	}
	return 0;
}


static double extech_read(int fd)
{
	struct packet p;
	fd_set read_fd;
	struct timeval tv;
	int ret;

	if (fd < 0)
		return 0.0;

	FD_ZERO(&read_fd);
	FD_SET(fd, &read_fd);

	tv.tv_sec = 0;
	tv.tv_usec = 500000;

	memset(&p, 0, sizeof(p));

	ret = select(fd + 1, &read_fd, NULL, NULL, &tv);
	if (ret <= 0)
		return -1;

	ret = read(fd, &p.buf, 250);
	if (ret < 0)
		return ret;
	p.len = ret;

	if (!parse_packet(&p))
		return p.watts;

	return -1000.0;
}

extech_power_meter::extech_power_meter(const char *extech_name)
{
	rate = 0.0;
	strncpy(dev_name, extech_name, sizeof(dev_name));
	int ret;

	fd = open_device(dev_name);
	if (fd < 0)
		return;

	ret = setup_serial_device(fd);
	if (ret) {
		close(fd);
		fd = -1;
		return;
	}
}


void extech_power_meter::measure(void)
{
	/* trigger the extech to send data */
	write(fd, " ", 1);

	rate = extech_read(fd);

}


void extech_power_meter::end_measurement(void)
{
	measure();
}

void extech_power_meter::start_measurement(void)
{
	/* ACPI battery state is a lagging indication, lets only measure at the end */
}


double extech_power_meter::joules_consumed(void)
{
	return rate;
}
