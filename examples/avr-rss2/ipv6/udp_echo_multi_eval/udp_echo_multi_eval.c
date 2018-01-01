/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/rtimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include "sys/ctimer.h"
#ifdef WITH_COMPOWER
#include "powertrace.h"
#endif
#include <stdio.h>
#include <string.h>

#include "dev/serial-line.h"
#include "net/ipv6/uip-ds6-route.h"

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 6000

#define UDP_EXAMPLE_ID  190

#define DEBUG DEBUG_DEBUG_NONE
#include "net/ip/uip-debug.h"

#ifndef PERIOD
#define PERIOD 1
#endif

#define START_TIME	0
#define END_TIME	1

#define NUMBER_OF_PACKETS	50
#define WAIT_INIT			(30 * CLOCK_SECOND)
#define MAX_PAYLOAD_LEN		30

static struct etimer periodic;
static int curr_pps;
static struct uip_udp_conn *client_conn[NUMBER_OF_PACKETS];
static uip_ipaddr_t server_ipaddr;


/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static int seq_id = 0;
static rtimer_clock_t eval_array[NUMBER_OF_PACKETS][2];

static void
tcpip_handler(void)
{
  int *reply_seq_id;
  if(uip_newdata()) {
	reply_seq_id = (int*)uip_appdata;
    eval_array[*reply_seq_id][END_TIME] = RTIMER_NOW();
    PRINTF("DATA recv seq_id:%d elapsed time: %d\n", *reply_seq_id, (int)eval_array[*reply_seq_id][END_TIME] - (int)eval_array[*reply_seq_id][START_TIME]);
  }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(/*void *ptr*/)
{
  char buf[MAX_PAYLOAD_LEN];

#ifdef SERVER_REPLY
  uint8_t num_used = 0;
  uip_ds6_nbr_t *nbr;

  nbr = nbr_table_head(ds6_neighbors);
  while(nbr != NULL) {
    nbr = nbr_table_next(ds6_neighbors, nbr);
    num_used++;
  }

  if(seq_id > 0) {
    ANNOTATE("#A r=%d/%d,color=%s,n=%d %d\n", reply, seq_id,
             reply == seq_id ? "GREEN" : "RED", uip_ds6_route_num_routes(), num_used);
  }
#endif /* SERVER_REPLY */

  if(seq_id < NUMBER_OF_PACKETS - 1)
	  seq_id++;
  else {
	  int i;
	  rtimer_clock_t elapsed_time;
	  printf("Packet Per Second: %d clock:%d\n", curr_pps, CLOCK_SECOND/curr_pps);
	  printf("SEQ:START:END:ELAPSED\n");
	  for(i=0; i < NUMBER_OF_PACKETS; i++) {

		  elapsed_time = eval_array[i][1] ? (eval_array[i][1] - eval_array[i][0]) : 0;
		  printf("%d:%lu:%lu:%lu\n", i, (unsigned long)eval_array[i][0], (unsigned long)eval_array[i][1], (unsigned long)elapsed_time);
	  }
	  seq_id = 0;
	  memset(eval_array, 0, 2*NUMBER_OF_PACKETS*sizeof(eval_array[0][0]));
	  if(curr_pps != CLOCK_SECOND)
	  {
		  PRINTF("Prev Packet Per Second:%d clock:%d\n", curr_pps, CLOCK_SECOND/curr_pps);
		  curr_pps = curr_pps*2;
		  etimer_set(&periodic, CLOCK_SECOND/curr_pps);
		  PRINTF("Curr Packet Per Second:%d clock:%d\n", curr_pps, CLOCK_SECOND/curr_pps);
	  }
	  else
		  etimer_stop(&periodic);
  }
  PRINTF("DATA send to ");
  PRINT6ADDR(&server_ipaddr);
  PRINTF(" 'Hello %d'\n", seq_id);
  memcpy(buf, &seq_id, sizeof(seq_id));
  //sprintf(buf, "Hello %d", seq_id);

  //INITIALIZE AN ARRAY IN THE BEGINNING with all ports! move to bottom

  uip_udp_packet_sendto(client_conn[seq_id], buf, sizeof(seq_id),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT+seq_id));
  eval_array[seq_id][START_TIME] = RTIMER_NOW();
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
	uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

/* The choice of server address determines its 6LoWPAN header compression.
 * (Our address will be compressed Mode 3 since it is derived from our
 * link-local address)
 * Obviously the choice made here must also be selected in udp-server.c.
 *
 * For correct Wireshark decoding using a sniffer, add the /64 prefix to the
 * 6LowPAN protocol preferences,
 * e.g. set Context 0 to fd00::. At present Wireshark copies Context/128 and
 * then overwrites it.
 * (Setting Context 0 to fd00::1111:2222:3333:4444 will report a 16 bit
 * compressed address of fd00::1111:22ff:fe33:xxxx)
 *
 * Note the IPCMV6 checksum verification depends on the correct uncompressed
 * addresses.
 */
 
#if 0
/* Mode 1 - 64 bits inline */
   uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 1);
#elif 1
/* Mode 2 - 16 bits inline */
  //uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
   uip_ip6addr(&server_ipaddr, 0x0000, 0, 0, 0, 0, 0xffff, 0xc0a8, 0x0197);
#else
/* Mode 3 - derived from server link-local (MAC) address */
  uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0x0250, 0xc2ff, 0xfea8, 0xcd1a); //redbee-econotag
#endif
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
#if WITH_COMPOWER
  static int print = 0;
#endif

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  set_global_address();

  PRINTF("UDP client process started nbr:%d routes:%d\n",
         NBR_TABLE_CONF_MAX_NEIGHBORS, UIP_CONF_MAX_ROUTES);

  print_local_addresses();

  /* 15s WAIT before start UDP connection */
  PRINTF("WAIT 15 sec before starting UDP client\n");
  etimer_set(&periodic, WAIT_INIT);
  PROCESS_YIELD();
  if(etimer_expired(&periodic)) {
	  int i;
	  for(i=0; i < NUMBER_OF_PACKETS; i++) {
		  /* new connection with remote host */
		  client_conn[i] = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT+i), NULL);
		  if(client_conn[i] == NULL) {
			  PRINTF("No UDP connection available, exiting the process!\n");
			  PROCESS_EXIT();
		  }
		  udp_bind(client_conn[i], UIP_HTONS(UDP_CLIENT_PORT));

		  PRINTF("Created a connection with the server ");
		  PRINT6ADDR(&client_conn[i]->ripaddr);
		  PRINTF(" local/remote port %u/%u\n",
				  UIP_HTONS(client_conn[i]->lport), UIP_HTONS(client_conn[i]->rport));
	  }



#if WITH_COMPOWER
  powertrace_sniff(POWERTRACE_ON);
#endif

} /* END: if(etimer_expired(&periodic) */

  curr_pps = 1;
  etimer_set(&periodic, CLOCK_SECOND/curr_pps);
  memset(eval_array, 0, 2*NUMBER_OF_PACKETS*sizeof(eval_array[0][0]));
  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }

    if(ev == serial_line_event_message && data != NULL) {
      char *str;
      str = data;
      if(str[0] == 'r') {
        uip_ds6_route_t *r;
        uip_ipaddr_t *nexthop;
        uip_ds6_defrt_t *defrt;
        uip_ipaddr_t *ipaddr;
        defrt = NULL;
        if((ipaddr = uip_ds6_defrt_choose()) != NULL) {
          defrt = uip_ds6_defrt_lookup(ipaddr);
        }
        if(defrt != NULL) {
          PRINTF("DefRT: :: -> %02d", defrt->ipaddr.u8[15]);
          PRINTF(" lt:%lu inf:%d\n", stimer_remaining(&defrt->lifetime),
                 defrt->isinfinite);
        } else {
          PRINTF("DefRT: :: -> NULL\n");
        }

        for(r = uip_ds6_route_head();
            r != NULL;
            r = uip_ds6_route_next(r)) {
          nexthop = uip_ds6_route_nexthop(r);
          PRINTF("Route: %02d -> %02d", r->ipaddr.u8[15], nexthop->u8[15]);
          /* PRINT6ADDR(&r->ipaddr); */
          /* PRINTF(" -> "); */
          /* PRINT6ADDR(nexthop); */
          PRINTF(" lt:%lu\n", r->state.lifetime);

        }
      }
    }

    if(etimer_expired(&periodic)) {
      etimer_reset(&periodic);
      send_packet();

#if WITH_COMPOWER
      if (print == 0) {
	powertrace_print("#P");
      }
      if (++print == 3) {
	print = 0;
      }
#endif

    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
