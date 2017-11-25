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
#ifndef TUNNEL_H
#define TUNNEL_H

#include "net/ip/uip.h"

void tunnel_init(void);
int tunnel_encap(const uint8_t *ipv6packet, const uint16_t ipv6len,
              uint8_t *resultpacket);
int tunnel_decap(const uint8_t *ipv4packet, const uint16_t ipv4len,
              uint8_t *resultpacket);

void tunnel_set_ipv4_address(const uip_ip4addr_t *ipv4addr,
                           const uip_ip4addr_t *netmask);
void tunnel_set_ipv6_address(const uip_ip6addr_t *ipv6addr);

const uip_ip4addr_t *tunnel_get_hostaddr(void);
const uip_ip4addr_t *tunnel_get_netmask(void);
const uip_ip4addr_t *tunnel_get_draddr(void);

void tunnel_set_hostaddr(const uip_ip4addr_t *hostaddr);
void tunnel_set_netmask(const uip_ip4addr_t *netmask);
void tunnel_set_draddr(const uip_ip4addr_t *draddr);

int tunnel_hostaddr_is_configured(void);

extern uint8_t *tunnel_packet_buffer;
extern uint16_t tunnel_packet_buffer_maxlen;

#include "tunnel-conf.h"

#ifndef TUNNEL_CONF_ETH_DRIVER
#error TUNNEL_CONF_ETH_DRIVER must be #defined in tunnel-conf.h
#else /* TUNNEL_CONF_ETH_DRIVER */
#define TUNNEL_ETH_DRIVER TUNNEL_CONF_ETH_DRIVER
#endif /* TUNNEL_CONF_ETH_DRIVER */

#ifndef TUNNEL_CONF_INPUT
#error TUNNEL_CONF_INPUT must be #defined in tunnel-conf.h
#else /* TUNNEL_CONF_INPUT */
#define TUNNEL_INPUT TUNNEL_CONF_INPUT
#endif /* TUNNEL_CONF_INPUT */

#ifndef TUNNEL_CONF_UIP_FALLBACK_INTERFACE
#error TUNNEL_CONF_UIP_FALLBACK_INTERFACE must be #defined in tunnel-conf.h
#else /* TUNNEL_CONF_UIP_FALLBACK_INTERFACE */
#define TUNNEL_UIP_FALLBACK_INTERFACE TUNNEL_CONF_UIP_FALLBACK_INTERFACE
#endif /* TUNNEL_CONF_UIP_FALLBACK_INTERFACE */

#ifdef TUNNEL_CONF_DHCP
#define TUNNEL_DHCP TUNNEL_CONF_DHCP
#else /* TUNNEL_CONF_DHCP */
/* Enable DHCP per default */
#define TUNNEL_DHCP 1
#endif /* TUNNEL_CONF_DHCP */

#endif /* TUNNEL_H */

