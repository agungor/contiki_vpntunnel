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
 * nat64.h
 * modified version of ip64 module in contiki
 *
 */

#ifndef NAT64_H_
#define NAT64_H_
#define EPHEMERAL_PORTRANGE 1024

#define IPV6_HDRLEN 40
#define IPV4_HDRLEN 20

#define IP_PROTO_ICMPV4  1
#define IP_PROTO_TCP     6
#define IP_PROTO_UDP     17
#define IP_PROTO_ICMPV6  58

#define ICMP_ECHO_REPLY  0
#define ICMP_ECHO        8
#define ICMP6_ECHO_REPLY 129
#define ICMP6_ECHO       128

#define BUFSIZE 600

#define PRINTF printf

#ifndef uip_ipaddr_copy
#define uip_ipaddr_copy(dest, src) (*(dest) = *(src))
#endif

typedef unsigned char  boolean;
typedef unsigned char  uint8_t;
typedef signed   char  int8_t;
typedef unsigned short uint16_t;
typedef signed   short int16_t;
typedef unsigned int   uint32_t;
typedef signed   int   int32_t;
typedef unsigned int   PointerType;

/**
 * Construct an IP address from four bytes.
 *
 * \param addr0 The first octet of the IP address.
 * \param addr1 The second octet of the IP address.
 * \param addr2 The third octet of the IP address.
 * \param addr3 The forth octet of the IP address.
 *
 * \hideinitializer
 */
#define uip_ipaddr(addr, addr0,addr1,addr2,addr3) do {  \
    (addr)->u8[0] = addr0;                              \
    (addr)->u8[1] = addr1;                              \
    (addr)->u8[2] = addr2;                              \
    (addr)->u8[3] = addr3;                              \
  } while(0)

/**
 * Construct an IPv6 address from eight 16-bit words.
 *
 * This function constructs an IPv6 address.
 *
 * \hideinitializer
 */
#define uip_ip6addr(addr, addr0,addr1,addr2,addr3,addr4,addr5,addr6,addr7) do { \
    (addr)->u16[0] = htons(addr0);                                      \
    (addr)->u16[1] = htons(addr1);                                      \
    (addr)->u16[2] = htons(addr2);                                      \
    (addr)->u16[3] = htons(addr3);                                      \
    (addr)->u16[4] = htons(addr4);                                      \
    (addr)->u16[5] = htons(addr5);                                      \
    (addr)->u16[6] = htons(addr6);                                      \
    (addr)->u16[7] = htons(addr7);                                      \
  } while(0)

#define uip_ip4addr_cmp(addr1, addr2) ((addr1)->u16[0] == (addr2)->u16[0] && \
                                       (addr1)->u16[1] == (addr2)->u16[1])
#define uip_ipaddr_maskcmp(addr1, addr2, mask)          \
  (((((uint16_t *)addr1)[0] & ((uint16_t *)mask)[0]) ==       \
    (((uint16_t *)addr2)[0] & ((uint16_t *)mask)[0])) &&      \
   ((((uint16_t *)addr1)[1] & ((uint16_t *)mask)[1]) ==       \
    (((uint16_t *)addr2)[1] & ((uint16_t *)mask)[1])))

#define uip_ipaddr_prefixcmp(addr1, addr2, length) (memcmp(addr1, addr2, length>>3) == 0)

/** \brief set IP address a to the link local all-nodes multicast address */
#define uip_create_linklocal_allnodes_mcast(a) uip_ip6addr(a, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001)

/**
 * Convert an IP address to four bytes separated by commas.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 printf("ipaddr=%d.%d.%d.%d\n", uip_ipaddr_to_quad(&ipaddr));
 \endcode
 *
 * \param a A pointer to a uip_ipaddr_t.
 * \hideinitializer
 */
#define uip_ipaddr_to_quad(a) (a)->u8[0],(a)->u8[1],(a)->u8[2],(a)->u8[3]

#define ip64_addr_is_ipv4_mapped_addr(a) \
  ((((a)->u16[0])  == 0) &&              \
   (((a)->u16[1])  == 0) &&              \
   (((a)->u16[2])  == 0) &&              \
   (((a)->u16[3])  == 0) &&              \
   (((a)->u16[4])  == 0) &&              \
   (((a)->u16[5])  == 0xFFFF))

struct tcp_hdr {
  uint16_t srcport;
  uint16_t destport;
  uint8_t seqno[4];
  uint8_t ackno[4];
  uint8_t tcpoffset;
  uint8_t flags;
  uint8_t  wnd[2];
  uint16_t tcpchksum;
  uint8_t urgp[2];
  uint8_t optdata[4];
};

struct udp_hdr {
  uint16_t srcport;
  uint16_t destport;
  uint16_t udplen;
  uint16_t udpchksum;
};

struct icmpv4_hdr {
  uint8_t type, icode;
  uint16_t icmpchksum;
};

struct icmpv6_hdr {
  uint8_t type, icode;
  uint16_t icmpchksum;
  uint16_t id, seqno;
};

typedef union {
  uint32_t u32[(BUFSIZE + 3) / 4];
  uint8_t u8[BUFSIZE];
} uip_buf_t;

/**
 * Representation of an IP address.
 *
 */
typedef union uip_ip4addr_t {
  uint8_t  u8[4];                       /* Initializer, must come first. */
  uint16_t u16[2];
} uip_ip4addr_t;

typedef union uip_ip6addr_t {
  uint8_t  u8[16];                      /* Initializer, must come first. */
  uint16_t u16[8];
} uip_ip6addr_t;

struct ipv6_hdr {
  uint8_t vtc;
  uint8_t tcflow;
  uint16_t flow;
  uint8_t len[2];
  uint8_t nxthdr, hoplim;
  uip_ip6addr_t srcipaddr, destipaddr;
};

struct ipv4_hdr {
  uint8_t vhl,
    tos,
    len[2],
    ipid[2],
    ipoffset[2],
    ttl,
    proto;
  uint16_t ipchksum;
  uip_ip4addr_t srcipaddr, destipaddr;
};

void ip64_init(void);
int nat64_6to4(const uint8_t *ipv6packet, const uint16_t ipv6len,
              uint8_t *resultpacket);

#endif /* NAT64_H_ */
