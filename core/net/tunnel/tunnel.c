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
 */

/* The packets sent to /received from Internet End Point (IEP) will be
 * handled in this module. A tunnel connection established between 6EP
 * and IEP via UDP protocol over IPv4. Any packet with a destination
 * address of outer subnet will be forwarded to IEP via tunnel. Original
 * source address/port and destination address/port of a packet will be
 * encapsulated into tunnel packet. Any packet received from IEP will
 * be decapsulated in this module and forwarded to sensor node addressed
 * in tunnel packet.

   +----------------+                                   +----------------+
   |                |                                   |                |
   |                | 	           +------+             |                |
   |  	  6EP       |--------------|tunnel|-------------|      IEP       |
   |                |  	           +------+		|                |
   |                |		   			|                |
   +----------------+		   internet		+----------------+

   IPv4 source address of 6EP have been obtained with DHCP from the IPv4
   network, but the exact way this has been obtained is outside the scope
   of the tunnel module. The IPv4 address is given to the tunnel module
   through the tunnel_set_ipv4_address() function.
*/

#include "tunnel.h"
#include "tunnel-addr.h"
#include "tunnel-addrmap.h"
#include "tunnel-conf.h"
#include "tunnel-special-ports.h"
#include "tunnel-eth-interface.h"
#include "tunnel-slip-interface.h"
#include "tunnel-dns64.h"
#include "net/ipv6/uip-ds6.h"
#include "tunnel-ipv4-dhcp.h"
#include "contiki-net.h"

#include "net/ip/uip-debug.h"

#include <string.h> /* for memcpy() */
#include <stdio.h> /* for printf() */

#define DEBUG 1

#if DEBUG
#undef PRINTF
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

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

#define EPHEMERAL_PORTRANGE 1024

#define IPV6_HDRLEN 	40
#define IPV4_HDRLEN 	20
#define ICMP4_HDRLEN	16
#define UDP_HDRLEN		8

#define IP_PROTO_ICMPV4  1
#define IP_PROTO_TCP     6
#define IP_PROTO_UDP     17
#define IP_PROTO_ICMPV6  58

#define ICMP_ECHO_REPLY  0
#define ICMP_ECHO        8
#define ICMP6_ECHO_REPLY 129
#define ICMP6_ECHO       128

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

#define BUFSIZE UIP_BUFSIZE

uip_buf_t tunnel_packet_buffer_aligned;
uint8_t *tunnel_packet_buffer = tunnel_packet_buffer_aligned.u8;

uint16_t tunnel_packet_buffer_maxlen = BUFSIZE;

static uip_ip4addr_t tunnel_hostaddr;
static uip_ip4addr_t tunnel_netmask;
static uip_ip4addr_t tunnel_draddr;

static uint16_t ipid;
static uint8_t tunnel_hostaddr_configured = 0;

static uip_ip6addr_t ipv6_local_address;
static uint8_t ipv6_local_address_configured = 0;

static uip_ip4addr_t ipv4_broadcast_addr;

/* Lifetimes for address mappings. */
#define SYN_LIFETIME     (CLOCK_SECOND * 20)
#define RST_LIFETIME     (CLOCK_SECOND * 30)
#define DEFAULT_LIFETIME (CLOCK_SECOND * 60 * 5)

/* TCP flag defines */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04

#define DNS_PORT 53

/*---------------------------------------------------------------------------*/
void
tunnel_init(void)
{
  int i;
  uint8_t state;

  uip_ipaddr(&ipv4_broadcast_addr, 255,255,255,255);
  tunnel_hostaddr_configured = 0;

  PRINTF("tunnel_init\n");
  TUNNEL_ETH_DRIVER.init();
#if TUNNEL_DHCP
  tunnel_ipv4_dhcp_init();
#endif /* TUNNEL_CONF_DHCP */

  /* Specify an IPv6 address for local communication to the
     host. We'll just pick the first one we find in our list. */
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    PRINTF("i %d used %d\n", i, uip_ds6_if.addr_list[i].isused);
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      tunnel_set_ipv6_address(&uip_ds6_if.addr_list[i].ipaddr);
      break;
    }
  }

}
/*---------------------------------------------------------------------------*/
void
tunnel_set_hostaddr(const uip_ip4addr_t *hostaddr)
{
  tunnel_hostaddr_configured = 1;
  tunnel_addr_copy4(&tunnel_hostaddr, hostaddr);
}
/*---------------------------------------------------------------------------*/
void
tunnel_set_netmask(const uip_ip4addr_t *netmask)
{
  tunnel_addr_copy4(&tunnel_netmask, netmask);
}
/*---------------------------------------------------------------------------*/
void
tunnel_set_draddr(const uip_ip4addr_t *draddr)
{
  tunnel_addr_copy4(&tunnel_draddr, draddr);
}
/*---------------------------------------------------------------------------*/
const uip_ip4addr_t *
tunnel_get_hostaddr(void)
{
  return &tunnel_hostaddr;
}
/*---------------------------------------------------------------------------*/
const uip_ip4addr_t *
tunnel_get_netmask(void)
{
  return &tunnel_netmask;
}
/*---------------------------------------------------------------------------*/
const uip_ip4addr_t *
tunnel_get_draddr(void)
{
  return &tunnel_draddr;
}
/*---------------------------------------------------------------------------*/
void
tunnel_set_ipv4_address(const uip_ip4addr_t *addr, const uip_ip4addr_t *netmask)
{
  tunnel_set_hostaddr(addr);
  tunnel_set_netmask(netmask);

  PRINTF("tunnel_set_ipv4_address: configuring address %d.%d.%d.%d/%d.%d.%d.%d\n",
	 tunnel_hostaddr.u8[0], tunnel_hostaddr.u8[1],
	 tunnel_hostaddr.u8[2], tunnel_hostaddr.u8[3],
	 tunnel_netmask.u8[0], tunnel_netmask.u8[1],
	 tunnel_netmask.u8[2], tunnel_netmask.u8[3]);
}
/*---------------------------------------------------------------------------*/
void
tunnel_set_ipv6_address(const uip_ip6addr_t *addr)
{
  tunnel_addr_copy6(&ipv6_local_address, (const uip_ip6addr_t *)addr);
  ipv6_local_address_configured = 1;
#if DEBUG
  PRINTF("tunnel_set_ipv6_address: configuring address ");
  uip_debug_ipaddr_print(addr);
  PRINTF("\n");
#endif /* DEBUG */
}
/*---------------------------------------------------------------------------*/
static uint16_t
chksum(uint16_t sum, const uint8_t *data, uint16_t len)
{
  uint16_t t;
  const uint8_t *dataptr;
  const uint8_t *last_byte;

  dataptr = data;
  last_byte = data + len - 1;

  while(dataptr < last_byte) {	/* At least two more bytes */
    t = (dataptr[0] << 8) + dataptr[1];
    sum += t;
    if(sum < t) {
      sum++;		/* carry */
    }
    dataptr += 2;
  }

  if(dataptr == last_byte) {
    t = (dataptr[0] << 8) + 0;
    sum += t;
    if(sum < t) {
      sum++;		/* carry */
    }
  }

  /* Return sum in host byte order. */
  return sum;
}
/*---------------------------------------------------------------------------*/
static uint16_t
ipv4_checksum(struct ipv4_hdr *hdr)
{
  uint16_t sum;

  sum = chksum(0, (uint8_t *)hdr, IPV4_HDRLEN);
  return (sum == 0) ? 0xffff : uip_htons(sum);
}
/*---------------------------------------------------------------------------*/
static uint16_t
ipv4_transport_checksum(const uint8_t *packet, uint16_t len, uint8_t proto)
{
  uint16_t transport_layer_len;
  uint16_t sum;
  struct ipv4_hdr *v4hdr = (struct ipv4_hdr *)packet;

  transport_layer_len = len - IPV4_HDRLEN;

  /* First sum pseudoheader. */

  if(proto != IP_PROTO_ICMPV4) {
    /* IP protocol and length fields. This addition cannot carry. */
    sum = transport_layer_len + proto;
    /* Sum IP source and destination addresses. */
    sum = chksum(sum, (uint8_t *)&v4hdr->srcipaddr, 2 * sizeof(uip_ip4addr_t));
  } else {
    /* ping replies' checksums are calculated over the icmp-part only */
    sum = 0;
  }

  /* Sum transport layer header and data. */
  sum = chksum(sum, &packet[IPV4_HDRLEN], transport_layer_len);

  return (sum == 0) ? 0xffff : uip_htons(sum);
}
/*---------------------------------------------------------------------------*/
static uint16_t
ipv6_transport_checksum(const uint8_t *packet, uint16_t len, uint8_t proto)
{
  uint16_t transport_layer_len;
  uint16_t sum;
  struct ipv6_hdr *v6hdr = (struct ipv6_hdr *)packet;

  transport_layer_len = len - IPV6_HDRLEN;

  /* First sum pseudoheader. */

  /* IP protocol and length fields. This addition cannot carry. */
  sum = transport_layer_len + proto;
  /* Sum IP source and destination addresses. */
  sum = chksum(sum, (uint8_t *)&v6hdr->srcipaddr, sizeof(uip_ip6addr_t));
  sum = chksum(sum, (uint8_t *)&v6hdr->destipaddr, sizeof(uip_ip6addr_t));

  /* Sum transport layer header and data. */
  sum = chksum(sum, &packet[IPV6_HDRLEN], transport_layer_len);

  return (sum == 0) ? 0xffff : uip_htons(sum);
}
/*---------------------------------------------------------------------------*/
int
tunnel_encap(const uint8_t *ipv6packet, const uint16_t ipv6packet_len,
	  uint8_t *resultpacket)
{
  struct ipv4_hdr *v4hdr;
  struct ipv6_hdr *v6hdr;
  struct udp_hdr *udphdr;
  struct tcp_hdr *tcphdr;
  struct icmpv4_hdr *icmpv4hdr;
  struct icmpv6_hdr *icmpv6hdr;
  uint16_t ipv6len, ipv4len;

  v6hdr = (struct ipv6_hdr *)ipv6packet;
  v4hdr = (struct ipv4_hdr *)resultpacket;

  if((v6hdr->len[0] << 8) + v6hdr->len[1] <= ipv6packet_len) {
    ipv6len = (v6hdr->len[0] << 8) + v6hdr->len[1] + IPV6_HDRLEN;
  } else {
    PRINTF("tunnel_encap: packet smaller than reported in IPv6 header, dropping\n");
    return 0;
  }

  /* We copy the data from the IPv6 packet into the IPv4 packet. We do
     not modify the data in any way. */
  //original packet ipv6 address 16 bytes
  //original packet port address 2 bytes
  PRINTF("tunnel_encap: packet received\n");

  memcpy(&resultpacket[IPV4_HDRLEN],
	 &ipv6packet[IPV6_HDRLEN],
	 ipv6len - IPV6_HDRLEN);

  udphdr = (struct udp_hdr *)&resultpacket[IPV4_HDRLEN];
  tcphdr = (struct tcp_hdr *)&resultpacket[IPV4_HDRLEN];
  icmpv4hdr = (struct icmpv4_hdr *)&resultpacket[IPV4_HDRLEN];
  icmpv6hdr = (struct icmpv6_hdr *)&ipv6packet[IPV6_HDRLEN];

  /* Translate the IPv6 header into an IPv4 header. */

  /* First the basics: the IPv4 version, header length, type of
     service, and offset fields. Those are the same for all IPv4
     packets we send, regardless of the values found in the IPv6
     packet. */
  v4hdr->vhl = 0x45;
  v4hdr->tos = 0;
  v4hdr->ipoffset[0] = v4hdr->ipoffset[1] = 0;

  /* We assume that the IPv6 packet has a fixed size header with no
     extension headers, and compute the length of the IPv4 packet and
     place the resulting value in the IPv4 packet header. */
  ipv4len = ipv6len - IPV6_HDRLEN + IPV4_HDRLEN;
  v4hdr->len[0] = ipv4len >> 8;
  v4hdr->len[1] = ipv4len & 0xff;

  /* For simplicity, we set a unique IP id for each outgoing IPv4
     packet. */
  ipid++;
  v4hdr->ipid[0] = ipid >> 8;
  v4hdr->ipid[1] = ipid & 0xff;

  /* Set the IPv4 protocol. We only support TCP, UDP, and ICMP at this
     point. While the IPv4 header protocol numbers are the same as the
     IPv6 next header numbers, the ICMPv4 and ICMPv6 numbers are
     different so we cannot simply copy the contents of the IPv6 next
     header field. */
  uint8_t prototype = v6hdr->nxthdr;
  switch(prototype) {
  case IP_PROTO_TCP:
    PRINTF("tunnel_encap: TCP header\n");
    v4hdr->proto = IP_PROTO_TCP;

    /* Compute and check the TCP checksum - since we're going to
       recompute it ourselves, we must ensure that it was correct in
       the first place. */
    if(ipv6_transport_checksum(ipv6packet, ipv6len,
                               IP_PROTO_TCP) != 0xffff) {
      PRINTF("Bad TCP checksum, dropping packet\n");
    }
    break;

  case IP_PROTO_UDP:
    PRINTF("tunnel_encap: UDP header\n");
    v4hdr->proto = IP_PROTO_UDP;

    /* Check if this is a DNS request. If so, we should rewrite it
       with the DNS64 module. */
    if(udphdr->destport == UIP_HTONS(DNS_PORT)) {
      tunnel_dns64_6to4((uint8_t *)v6hdr + IPV6_HDRLEN + sizeof(struct udp_hdr),
                      ipv6len - IPV6_HDRLEN - sizeof(struct udp_hdr),
                      (uint8_t *)udphdr + sizeof(struct udp_hdr),
                      BUFSIZE - IPV4_HDRLEN - sizeof(struct udp_hdr));
    }
    /* Compute and check the UDP checksum - since we're going to
       recompute it ourselves, we must ensure that it was correct in
       the first place. */
    if(ipv6_transport_checksum(ipv6packet, ipv6len,
                               IP_PROTO_UDP) != 0xffff) {
      PRINTF("tunnel_encap: Bad UDP checksum, dropping packet\n");
    }
    break;

  case IP_PROTO_ICMPV6:
    PRINTF("tunnel_encap: ICMPv6 header\n");
    v4hdr->proto = IP_PROTO_ICMPV4;
    /* Translate only ECHO_REPLY messages. */
    if(icmpv6hdr->type == ICMP6_ECHO_REPLY) {
      icmpv4hdr->type = ICMP_ECHO_REPLY;
    } else if(icmpv6hdr->type == ICMP6_ECHO) {
    	icmpv4hdr->type = ICMP_ECHO;
    }
    else{
      PRINTF("tunnel_encap: ICMPv6 mapping for type %d not implemented.\n",
	     icmpv6hdr->type);
      return 0;
    }
    break;

  default:
    /* We did not recognize the next header, and we do not attempt to
       translate something we do not understand, so we return 0 to
       indicate that no successful translation could be made. */
    PRINTF("tunnel_encap: Could not convert IPv6 next hop %d to an IPv4 protocol number.\n",
	   v6hdr->nxthdr);
    return 0;
  }

  /* We set the IPv4 ttl value to the hoplim number from the IPv6
     header. This means that information about the IPv6 topology is
     transported into to the IPv4 network. */
  v4hdr->ttl = v6hdr->hoplim;


  /* We next convert the destination address. We make this conversion
     with the tunnel_addr_6to4() function. If the conversion
     fails, tunnel_addr_6to4() returns 0. If so, we also return 0 to
     indicate failure. */
  if(tunnel_addr_6to4(&v6hdr->destipaddr,
		    &v4hdr->destipaddr) == 0) {
#if DEBUG
    PRINTF("tunnel_encap: Could not convert IPv6 destination address.\n");
    uip_debug_ipaddr_print(&v6hdr->destipaddr);
    PRINTF("\n");
#endif /* DEBUG */
    return 0;
  }

  /* We set the source address in the IPv4 packet to be the IPv4
     address that we have been configured with through the
     tunnel_set_ipv4_address() function. Only let broadcasts through. */
  if(!tunnel_hostaddr_configured &&
     !uip_ip4addr_cmp(&v4hdr->destipaddr, &ipv4_broadcast_addr)) {
    PRINTF("tunnel_encap: no IPv4 address configured.\n");
    return 0;
  }
  tunnel_addr_copy4(&v4hdr->srcipaddr, &tunnel_hostaddr);


  /* Next we update the transport layer header. This must be updated
     in two ways: the source/dest addr and port number will be changed
     and the transport layer checksum must be recomputed. Source address/port
     and destination address/port will be obtained from the decapsulated
     tunnel data.
  */

  if((uip_ntohs(udphdr->srcport) >= EPHEMERAL_PORTRANGE))
  {
	  uip_ip6addr_t dest_addr;
	  uip_ip6addr_t src_addr;
	  uint16_t dest_port = 0;
	  uint16_t src_port = 0;
	  uint16_t udplen;

	  //set IEP ipv4 address
	  tunnel_addr_set_dest(&v4hdr->destipaddr, TUNNEL_DST_ADDR0, TUNNEL_DST_ADDR1, TUNNEL_DST_ADDR2, TUNNEL_DST_ADDR3);

	  if(prototype == IP_PROTO_UDP || prototype == IP_PROTO_TCP) {
		  dest_port = udphdr->destport;
		  src_port = udphdr->srcport;

	  }
	  else if(prototype == IP_PROTO_ICMPV6) {
		  memmove(&resultpacket[IPV4_HDRLEN + UDP_HDRLEN],
		  	  	 &resultpacket[IPV4_HDRLEN],
		  	  	 ipv6len - IPV6_HDRLEN);
		  udphdr = (struct udp_hdr *)&resultpacket[IPV4_HDRLEN];
		  udphdr->udplen = uip_htons(ipv4len - ICMP4_HDRLEN - IPV4_HDRLEN);
		  ipv4len += UDP_HDRLEN;
	  }
	  //store original source and destination address & port of the packet
	  dest_addr = v6hdr->destipaddr;
	  src_addr = v6hdr->srcipaddr;

	  //source ipv6 address (16 byte) + port number (2 bytes)
	  //dest ipv6 address (16 byte) + port number (2 bytes)
	  //total size = 36 bytes
	  ipv4len += 36;
	  v4hdr->len[0] = ipv4len >> 8;
	  v4hdr->len[1] = ipv4len & 0xff;
	  v4hdr->proto = IP_PROTO_UDP;

	  //set ipv4 tunnel packet udp packet source & dest port
	  udphdr->srcport = uip_htons(TUNNEL_SRC_PORT);
	  udphdr->destport = uip_htons(TUNNEL_DST_PORT);
	  udplen = uip_ntohs(udphdr->udplen);
	  udplen += 36;
	  udphdr->udplen = uip_htons(udplen);

	  uint8_t payload_start = IPV4_HDRLEN + UDP_HDRLEN;

	  //insert tunnel data (source ip/port dest ip/port) to beginning of payload
	  memmove(&resultpacket[payload_start+36],
	  	 &resultpacket[payload_start],
	  	 ipv6len - IPV6_HDRLEN);
	  memcpy(&resultpacket[payload_start],
			  &src_addr,
			  sizeof(src_addr));
	  resultpacket[payload_start+16] = src_port & 0xff;
	  resultpacket[payload_start+17] = src_port >> 8;
	  memcpy(&resultpacket[payload_start+18],
			  &dest_addr,
			  sizeof(dest_addr));
	  resultpacket[payload_start+34] = dest_port & 0xff;
	  resultpacket[payload_start+35] = dest_port >> 8;
  }
  else
	  PRINTF("packet in EPHEMERAL_PORTRANGE srcport: %d\n", uip_ntohs(udphdr->srcport));


  /* The IPv4 header is now complete, so we can compute the IPv4
       header checksum. */
    v4hdr->ipchksum = 0;
    v4hdr->ipchksum = ~(ipv4_checksum(v4hdr));

    /* The checksum is in different places in the different protocol
       headers, so we need to be sure that we update the correct
       field. */
    switch(v4hdr->proto) {
    case IP_PROTO_TCP:
      tcphdr->tcpchksum = 0;
      tcphdr->tcpchksum = ~(ipv4_transport_checksum(resultpacket, ipv4len,
  						  IP_PROTO_TCP));
      break;
    case IP_PROTO_UDP:
      udphdr->udpchksum = 0;
      udphdr->udpchksum = ~(ipv4_transport_checksum(resultpacket, ipv4len,
  						  IP_PROTO_UDP));
      if(udphdr->udpchksum == 0) {
        udphdr->udpchksum = 0xffff;
      }
      break;
    case IP_PROTO_ICMPV4:
      icmpv4hdr->icmpchksum = 0;
      icmpv4hdr->icmpchksum = ~(ipv4_transport_checksum(resultpacket, ipv4len,
  						      IP_PROTO_ICMPV4));
      break;

    default:
      PRINTF("tunnel_encap: transport protocol %d not implemented\n", v4hdr->proto);
      return 0;
    }

  /* Finally, we return the length of the resulting IPv4 packet. */
  PRINTF("tunnel_encap: ipv4len %d\n", ipv4len);
  return ipv4len;
}
/*---------------------------------------------------------------------------*/
int
tunnel_decap(const uint8_t *ipv4packet, const uint16_t ipv4packet_len,
	  uint8_t *resultpacket)
{
  struct ipv4_hdr *v4hdr;
  struct ipv6_hdr *v6hdr;
  struct udp_hdr *udphdr;
  struct tcp_hdr *tcphdr;
  struct icmpv4_hdr *icmpv4hdr;
  struct icmpv6_hdr *icmpv6hdr;
  uint16_t ipv4len, ipv6len, ipv6_packet_len;

  v6hdr = (struct ipv6_hdr *)resultpacket;
  v4hdr = (struct ipv4_hdr *)ipv4packet;

  if((v4hdr->len[0] << 8) + v4hdr->len[1] <= ipv4packet_len) {
    ipv4len = (v4hdr->len[0] << 8) + v4hdr->len[1];
  } else {
    PRINTF("tunnel_decap: packet smaller than reported in IPv4 header, dropping\n");
    return 0;
  }

  if(ipv4len <= IPV4_HDRLEN) {
    return 0;
  }

  /* Make sure that the resulting packet fits in the tunnel packet
     buffer. If not, we drop it. */
  if(ipv4len - IPV4_HDRLEN + IPV6_HDRLEN > BUFSIZE) {
    PRINTF("tunnel_decap: packet too big to fit in buffer, dropping\n");
    return 0;
  }
  /* We copy the data from the IPv4 packet into the IPv6 packet. */
  memcpy(&resultpacket[IPV6_HDRLEN],
	 &ipv4packet[IPV4_HDRLEN],
	 ipv4len - IPV4_HDRLEN);
  
  udphdr = (struct udp_hdr *)&resultpacket[IPV6_HDRLEN];
  tcphdr = (struct tcp_hdr *)&resultpacket[IPV6_HDRLEN];
  icmpv4hdr = (struct icmpv4_hdr *)&ipv4packet[IPV4_HDRLEN];
  icmpv6hdr = (struct icmpv6_hdr *)&resultpacket[IPV6_HDRLEN];

  ipv6len = ipv4len - IPV4_HDRLEN + IPV6_HDRLEN;
  ipv6_packet_len = ipv6len - IPV6_HDRLEN;

  /* Translate the IPv4 header into an IPv6 header. */

  /* We first fill in the simple fields: IP header version, traffic
     class and flow label, and length fields. */
  v6hdr->vtc = 0x60;
  v6hdr->tcflow = 0;
  v6hdr->flow = 0;
  v6hdr->len[0] = ipv6_packet_len >> 8;
  v6hdr->len[1] = ipv6_packet_len & 0xff;

  /* We use the IPv4 TTL field as the IPv6 hop limit field. */
  v6hdr->hoplim = v4hdr->ttl;

  
  /* We now translate the IPv4 source and destination addresses to
     IPv6 source and destination addresses. We translate the IPv4
     source address into an IPv6-encoded IPv4 address. The IPv4
     destination address will be the address with which we have
     previously been configured, through the tunnel_set_ipv4_address()
     function. */
  if(tunnel_addr_4to6(&v4hdr->srcipaddr, &v6hdr->srcipaddr) == 0) {
    PRINTF("tunnel_packet_decap: failed to convert source IP address\n");
    return 0;
  }

    /* For the next header field, we simply use the IPv4 protocol
     field. We only support UDP and TCP packets. */
  switch(v4hdr->proto) {
  case IP_PROTO_UDP:
    v6hdr->nxthdr = IP_PROTO_UDP;
    /* Check if this is a DNS request. If so, we should rewrite it
       with the DNS64 module. */
    if(udphdr->srcport == UIP_HTONS(DNS_PORT)) {
      int len;

      len = tunnel_dns64_4to6((uint8_t *)v4hdr + IPV4_HDRLEN + sizeof(struct udp_hdr),
                            ipv4len - IPV4_HDRLEN - sizeof(struct udp_hdr),
                            (uint8_t *)v6hdr + IPV6_HDRLEN + sizeof(struct udp_hdr),
                            ipv6_packet_len - sizeof(struct udp_hdr));
      ipv6_packet_len = len + sizeof(struct udp_hdr);
      v6hdr->len[0] = ipv6_packet_len >> 8;
      v6hdr->len[1] = ipv6_packet_len & 0xff;
      ipv6len = ipv6_packet_len + IPV6_HDRLEN;

    }
    break;

  case IP_PROTO_TCP:
    v6hdr->nxthdr = IP_PROTO_TCP;
    break;

  case IP_PROTO_ICMPV4:
    /* Allow only ICMPv4 ECHO_REQUESTS (ping packets) through to the
       local IPv6 host. */
    if(icmpv4hdr->type == ICMP_ECHO) {
      PRINTF("tunnel_decap: translating ICMPv4 ECHO packet\n");
      v6hdr->nxthdr = IP_PROTO_ICMPV6;
      icmpv6hdr->type = ICMP6_ECHO;
      tunnel_addr_copy6(&v6hdr->destipaddr, &ipv6_local_address);
    } else {
      PRINTF("tunnel_packet_decap: ICMPv4 packet type %d not supported\n",
	     icmpv4hdr->type);
      return 0;
    }
    break;

  default:
    /* For protocol types that we do not support, we return 0 to
       indicate that we failed to translate the packet to an IPv6
       packet. */
    PRINTF("tunnel_packet_decap: protocol type %d not supported\n",
	   v4hdr->proto);
    return 0;
  }

  /* Translate IPv4 broadcasts to IPv6 all-nodes multicasts. */
  if(uip_ip4addr_cmp(&v4hdr->destipaddr, &ipv4_broadcast_addr) ||
     (uip_ipaddr_maskcmp(&v4hdr->destipaddr, &tunnel_hostaddr,
			 &tunnel_netmask) &&
      ((v4hdr->destipaddr.u16[0] & (~tunnel_netmask.u16[0])) ==
       (ipv4_broadcast_addr.u16[0] & (~tunnel_netmask.u16[0]))) &&
      ((v4hdr->destipaddr.u16[1] & (~tunnel_netmask.u16[1])) ==
       (ipv4_broadcast_addr.u16[1] & (~tunnel_netmask.u16[1]))))) {
    uip_create_linklocal_allnodes_mcast(&v6hdr->destipaddr);
  } else {

    if(!tunnel_hostaddr_configured) {
      PRINTF("tunnel_packet_decap: no local IPv4 address configured, dropping incoming packet.\n");
      return 0;
    }

    if(!uip_ip4addr_cmp(&v4hdr->destipaddr, &tunnel_hostaddr)) {
      PRINTF("tunnel_packet_decap: the IPv4 destination address %d.%d.%d.%d did not match our IPv4 address %d.%d.%d.%d\n",
	     uip_ipaddr_to_quad(&v4hdr->destipaddr),
	     uip_ipaddr_to_quad(&tunnel_hostaddr));
      return 0;
    }

    /* Packets with ports within EPHEMERAL_PORTRANGE handled locally */
    if((v4hdr->proto == IP_PROTO_TCP || v4hdr->proto == IP_PROTO_UDP)) {
    	if(uip_htons(tcphdr->destport) < EPHEMERAL_PORTRANGE) {
    		/* This packet should go to the local host. */
    		PRINTF("Port is in the non-ephemeral port range %d (%d)\n",
    				tcphdr->destport, uip_htons(tcphdr->destport));
    		tunnel_addr_copy6(&v6hdr->destipaddr, &ipv6_local_address);
    	}
    }
  }

  if((uip_ntohs(udphdr->srcport) >= EPHEMERAL_PORTRANGE))
  {
	  uip_ip6addr_t dest_addr;
	  uip_ip6addr_t src_addr;
	  uint16_t dest_port;
	  uint16_t src_port;
	  uint16_t udplen;

	  uint8_t payload_start = IPV6_HDRLEN + UDP_HDRLEN;

	  memcpy(&src_addr,
			  &resultpacket[payload_start],
			  sizeof(src_addr));
	  memcpy(&src_port,
			  &resultpacket[payload_start+16],
			  sizeof(src_port));
	  memcpy(&dest_addr,
			  &resultpacket[payload_start+18],
			  sizeof(dest_addr));
	  memcpy(&dest_port,
			  &resultpacket[payload_start+34],
			  sizeof(dest_port));
	  //remove tunnel data (source ip/port dest ip/port)
	  memmove(&resultpacket[payload_start],
			  &resultpacket[payload_start+36],
			  ipv4len - IPV4_HDRLEN - UDP_HDRLEN);

	  //source ipv6 address (16 byte) + port number (2 bytes)
	  //dest ipv6 address (16 byte) + port number (2 bytes)
	  //total size = 36 bytes
	  ipv6_packet_len -= 36;
	  v6hdr->len[0] = ipv6_packet_len >> 8;
	  v6hdr->len[1] = ipv6_packet_len & 0xff;
	  ipv6len = ipv6_packet_len + IPV6_HDRLEN;

	  udplen = uip_ntohs(udphdr->udplen);
	  udplen -= 36;
	  udphdr->udplen = uip_htons(udplen);

	  udphdr->srcport = src_port;
	  udphdr->destport = dest_port;

	  tunnel_addr_copy6(&v6hdr->destipaddr, &dest_addr);
	  tunnel_addr_copy6(&v6hdr->srcipaddr, &src_addr);
  }
  else
	  PRINTF("packet in EPHEMERAL_PORTRANGE srcport: %d\n", uip_ntohs(udphdr->srcport));

  /* The checksum is in different places in the different protocol
       headers, so we need to be sure that we update the correct
       field. */
    switch(v6hdr->nxthdr) {
    case IP_PROTO_TCP:
      tcphdr->tcpchksum = 0;
      tcphdr->tcpchksum = ~(ipv6_transport_checksum(resultpacket,
  						  ipv6len,
  						  IP_PROTO_TCP));
      break;
    case IP_PROTO_UDP:
      udphdr->udpchksum = 0;
      udphdr->udpchksum = ~(ipv6_transport_checksum(resultpacket,
  						  ipv6len,
  						  IP_PROTO_UDP));
      if(udphdr->udpchksum == 0) {
        udphdr->udpchksum = 0xffff;
      }
      break;

    case IP_PROTO_ICMPV6:
      icmpv6hdr->icmpchksum = 0;
      icmpv6hdr->icmpchksum = ~(ipv6_transport_checksum(resultpacket,
                                                  ipv6len,
                                                  IP_PROTO_ICMPV6));
      break;
    default:
      PRINTF("tunnel_decap: transport protocol %d not implemented\n", v4hdr->proto);
      return 0;
    }

  /* Finally, we return the length of the resulting IPv6 packet. */
  PRINTF("tunnel_decap: ipv6len %d\n", ipv6len);
  return ipv6len;
}
/*---------------------------------------------------------------------------*/
int
tunnel_hostaddr_is_configured(void)
{
  return tunnel_hostaddr_configured;
}
/*---------------------------------------------------------------------------*/
static void
interface_init(void)
{
  TUNNEL_UIP_FALLBACK_INTERFACE.init();
}
/*---------------------------------------------------------------------------*/
static int
interface_output(void)
{
  PRINTF("tunnel: interface_output len %d\n", uip_len);
  TUNNEL_UIP_FALLBACK_INTERFACE.output();

  return 0;
}
/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface tunnel_uip_fallback_interface = {
  interface_init, interface_output
};

