/*
 * Copyright (c) 2017, Copyright Arman Gungor
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Author  : Arman Gungor gungor@kth.se
 * Created : 2017-12-03
 *
 * tunproxy.c 6EP tunneling over UDP with tun/tap
 * modified version of http://www.secdev.org/projects/tuntap_udp/files/tunproxy.c
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <getopt.h>
#include <sys/ioctl.h>

#include "nat64.h"

#define PERROR(x) do { perror(x); exit(1); } while (0)
#define ERROR(x, args ...) do { fprintf(stderr,"ERROR:" x, ## args); exit(1); } while (0)

void usage()
{
	fprintf(stderr, "Usage: tunproxy [-s port|-c targetip:port] [-e]\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	struct sockaddr_in sin, sout, from;
	struct ifreq ifr;
	int fd, s, fromlen, soutlen, port, PORT, l, offset;
	char c, *p, *ip;
	char bufin[1504];
	char bufout[1504];
	int ethheaderlen = 14;
	char ethheader[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
		                 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
						 0x08, 0x00 };
	fd_set fdset;
	

	int TUNMODE = IFF_TUN, DEBUG = 1;

	//default settings
	PORT = 9000;

	while ((c = getopt(argc, argv, "s:c:ehd")) != -1) {
		switch (c) {
		case 'h':
			usage();
		case 'd':
			DEBUG++;
			break;
		case 's':
			PORT = atoi(optarg);
			break;
		case 'c':
			p = memchr(optarg,':',16);
			if (!p) ERROR("invalid argument : [%s]\n",optarg);
			*p = 0;
			ip = optarg;
			port = atoi(p+1);
			PORT = 0;
			break;
		case 'e':
			TUNMODE = IFF_TAP;
			break;
		default:
			usage();
		}
	}

	//create and initialize tun interface socket
	if ( (fd = open("/dev/net/tun",O_RDWR)) < 0) PERROR("open");

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = TUNMODE | IFF_NO_PI;
	strncpy(ifr.ifr_name, "toto%d", IFNAMSIZ);
	if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) PERROR("ioctl");

	printf("Allocated interface %s. Configure and use it\n", ifr.ifr_name);
	
	//create and initialize udp tunnel socket to communicate with 6EP
	s = socket(PF_INET, SOCK_DGRAM, 0);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(PORT);
	if ( bind(s,(struct sockaddr *)&sin, sizeof(sin)) < 0) PERROR("bind");

	fromlen = sizeof(from);

	nat64_init();

	offset = 0;
	bzero(bufout, offset);
	if(TUNMODE == IFF_TAP) {
		memcpy(&bufout[offset], ethheader, ethheaderlen);
		offset += ethheaderlen;

	}

	//receive first packet to establish tunnel
	while(1) {
		l = recvfrom(s, bufin, sizeof(bufin), 0, (struct sockaddr *)&from, &fromlen);
		if (l < 0) PERROR("recvfrom");
		break;
	}

	//recv packet from 6EP and send them to tun interface and vice versa.
	while (1) {
		FD_ZERO(&fdset);
		//add tun interface and udp tunnel sockets to select set
		FD_SET(fd, &fdset);
		FD_SET(s, &fdset);
		if (select(fd+s+1, &fdset,NULL,NULL,NULL) < 0) PERROR("select");
		if (FD_ISSET(fd, &fdset)) {
			//read packet from tun interface
			//if (DEBUG) write(1,">", 1);
			l = read(fd, bufin, sizeof(bufin));
			//packet received from internet translate to IPv6 packet
			if (l < 0) PERROR("read");
		    l = ip64_4to6(bufin, l, bufout);
			if (sendto(s, bufout, l, 0, (struct sockaddr *)&from, fromlen) < 0) PERROR("sendto");
		} else {
			//if (DEBUG) write(1,"<", 1);
			l = recvfrom(s, &bufin, (sizeof(bufin)), 0, (struct sockaddr *)&sout, &soutlen);
			printf("recvfrom l:%d\n",l);
			//packet received from 6EP translate to IPv4 packet
			l = ip64_6to4(bufin, l, &bufout[offset]);
			//write resulting packet to tun interface
			printf("write fd:%d l:%d\n", fd,l);
			if (write(fd, bufout, l+offset) < 0) PERROR("write");
		}
	}
}
	       
	
