/*
 * Copyright (c) 2012, Thingsquare, http://www.thingsquare.com/.
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
#include "contiki.h"
#include "contiki-net.h"
#include "tunnel.h"

#include <stdio.h>

#include "tunnel-dhcpc.h"
#include "tunnel-eth.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

PROCESS(tunnel_ipv4_dhcp_process, "IPv4 DHCP");

uip_ipaddr_t uip_hostaddr; /* Needed because it is referenced by dhcpc.c */


/*---------------------------------------------------------------------------*/
void
tunnel_ipv4_dhcp_init(void)
{
  printf("IP64: Starting DHCPv4\n");
  process_start(&tunnel_ipv4_dhcp_process, NULL);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tunnel_ipv4_dhcp_process, ev, data)
{
  PROCESS_BEGIN();

  tunnel_dhcpc_init(&tunnel_eth_addr, sizeof(tunnel_eth_addr));

  PRINTF("IP64: Inited\n");

  tunnel_dhcpc_request();
  PRINTF("IP64: Requested\n");
  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == tcpip_event ||
       ev == PROCESS_EVENT_TIMER) {
      tunnel_dhcpc_appcall(ev, data);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
tunnel_dhcpc_configured(const struct tunnel_dhcpc_state *s)
{
  uip_ip6addr_t ip6dnsaddr;
  PRINTF("IP64: DHCP Configured with %d.%d.%d.%d\n",
	 s->ipaddr.u8[0], s->ipaddr.u8[1],
	 s->ipaddr.u8[2], s->ipaddr.u8[3]);

  tunnel_set_hostaddr((uip_ip4addr_t *)&s->ipaddr);
  tunnel_set_netmask((uip_ip4addr_t *)&s->netmask);
  tunnel_set_draddr((uip_ip4addr_t *)&s->default_router);
}
/*---------------------------------------------------------------------------*/
void
tunnel_dhcpc_unconfigured(const struct tunnel_dhcpc_state *s)
{
}
/*---------------------------------------------------------------------------*/
