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
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/resolv.h"

#include <string.h>
#include <stdbool.h>

//#undef DEBUG
//#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

#include "ccn-lite-contiki.h"

#define SEND_INTERVAL		6 * CLOCK_SECOND//15 * CLOCK_SECOND
#define MAX_PAYLOAD_LEN		40

static struct uip_udp_conn *client_conn;

#define PRE_TRY_CONNCETION 0
#define ENABLE_CCNL

//#define CFS_DEBUG

#define CCNL_MAKE_INTEREST

/*---------------------------------------------------------------------------*/
PROCESS(ccn_client_process, "ccn client process");
AUTOSTART_PROCESSES(&resolv_process,&ccn_client_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  char *str;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    printf("Response from the mote2: '%s'\n", str);
  }
}
/*---------------------------------------------------------------------------*/
#ifdef ENABLE_CCNL
#ifdef CCNL_MAKE_INTEREST
#define BUF_SIZE    (108)
#else
#define BUF_SIZE    (128)
#endif
static unsigned char _int_buf[BUF_SIZE];
#endif

#ifdef CFS_DEBUG
#include "cfs/cfs.h"
static char *filename = "msg_file";
static int fd_write;
static int n;
#endif

static char buf[MAX_PAYLOAD_LEN];

static void
ccnl_generation(void)
{
  static int seq_id;
  if(seq_id == 4) seq_id=0;

  printf("mote1 sending to: ");
  PRINT6ADDR(&client_conn->ripaddr);
//  sprintf(buf, "Hello %d from the client", ++seq_id);
  sprintf(buf, "kista/kth/sics/alice/%d", ++seq_id);
  printf(" (msg: %s)\n", buf);
#if SEND_TOO_LARGE_PACKET_TO_TEST_FRAGMENTATION
    uip_udp_packet_send(client_conn, buf, UIP_APPDATA_SIZE);
#else /* SEND_TOO_LARGE_PACKET_TO_TEST_FRAGMENTATION */
#ifdef ENABLE_CCNL
    int suite = CCNL_SUITE_NDNTLV;
//    int suite = CCNL_SUITE_CCNTLV;
    int len, offs;
    memset(_int_buf, '\0', BUF_SIZE);
#ifdef CCNL_MAKE_INTEREST
    offs = ccnl_make_interest(suite, buf, NULL,_int_buf, BUF_SIZE, &len);
  	if(offs == -1){
  		printf("failed to make interest!\n");
  	}
  	uip_udp_packet_send(client_conn, _int_buf, len/*strlen(_int_buf)*/);
#else
    unsigned char *data;
  	char* content="5635\0";
  	offs = ccnl_make_content(suite, buf, content, NULL,_int_buf, &len);
  	if(offs == -1){
  		printf("failed to make content!\n");
  	}
  	data = _int_buf + offs;
  	uip_udp_packet_send(client_conn, data, len);
#endif

#ifdef CFS_DEBUG
    fd_write = cfs_open(filename, CFS_WRITE | CFS_APPEND);
    if(fd_write != -1) {
      n = cfs_write(fd_write, data, len);
      cfs_close(fd_write);
      printf("successfully appended data to cfs. wrote %i bytes  \n",n);
    } else {
      printf("ERROR: could not write to memory.\n");
    }
#endif
#else
  	uip_udp_packet_send(client_conn, buf, strlen(buf));
#ifdef CFS_DEBUG
    fd_write = cfs_open(filename, CFS_WRITE | CFS_APPEND);
    if(fd_write != -1) {
      n = cfs_write(fd_write, buf, sizeof(buf));
      cfs_close(fd_write);
      printf("successfully appended data to cfs. wrote %i bytes  \n",n);
    } else {
      printf("ERROR: could not write to memory.\n");
    }
#endif
	#endif
#endif /* SEND_TOO_LARGE_PACKET_TO_TEST_FRAGMENTATION */
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
    }
  }
}
/*---------------------------------------------------------------------------*/
#if UIP_CONF_ROUTER
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
}
#endif /* UIP_CONF_ROUTER */
/*---------------------------------------------------------------------------*/
#if PRE_TRY_CONNCETION
static resolv_status_t
set_connection_address(uip_ipaddr_t *ipaddr)
{
#ifndef UDP_CONNECTION_ADDR
#if RESOLV_CONF_SUPPORTS_MDNS
#define UDP_CONNECTION_ADDR       contiki-udp-server.local
#elif UIP_CONF_ROUTER
#define UDP_CONNECTION_ADDR       aaaa:0:0:0:d28c:7bff:fe15:11
#else
#define UDP_CONNECTION_ADDR       fe80:0:0:0:6466:6666:6666:6666
#endif
#endif /* !UDP_CONNECTION_ADDR */

#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)

  resolv_status_t status = RESOLV_STATUS_ERROR;

  if(uiplib_ipaddrconv(QUOTEME(UDP_CONNECTION_ADDR), ipaddr) == 0) {
    uip_ipaddr_t *resolved_addr = NULL;
    status = resolv_lookup(QUOTEME(UDP_CONNECTION_ADDR),&resolved_addr);
    if(status == RESOLV_STATUS_UNCACHED || status == RESOLV_STATUS_EXPIRED) {
      PRINTF("Attempting to look up %s\n",QUOTEME(UDP_CONNECTION_ADDR));
      resolv_query(QUOTEME(UDP_CONNECTION_ADDR));
      status = RESOLV_STATUS_RESOLVING;
    } else if(status == RESOLV_STATUS_CACHED && resolved_addr != NULL) {
      PRINTF("Lookup of \"%s\" succeded!\n",QUOTEME(UDP_CONNECTION_ADDR));
    } else if(status == RESOLV_STATUS_RESOLVING) {
      PRINTF("Still looking up \"%s\"...\n",QUOTEME(UDP_CONNECTION_ADDR));
    } else {
      PRINTF("Lookup of \"%s\" failed. status = %d\n",QUOTEME(UDP_CONNECTION_ADDR),status);
    }
    if(resolved_addr)
      uip_ipaddr_copy(ipaddr, resolved_addr);
  } else {
    status = RESOLV_STATUS_CACHED;
  }

  return status;
}
#endif
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ccn_client_process, ev, data)
{
  static struct etimer et;
  uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();
  PRINTF("ccn client process started\n");

#if UIP_CONF_ROUTER
  set_global_address();
#endif

  print_local_addresses();
#if PRE_TRY_CONNCETION
  static resolv_status_t status = RESOLV_STATUS_UNCACHED;
  while(status != RESOLV_STATUS_CACHED) {
    status = set_connection_address(&ipaddr);

    if(status == RESOLV_STATUS_RESOLVING) {
      PROCESS_WAIT_EVENT_UNTIL(ev == resolv_event_found);
    } else if(status != RESOLV_STATUS_CACHED) {
      PRINTF("Can't get connection address.\n");
      PROCESS_YIELD();
    }
  }
#else
  static char *server="fd00::212:4b00:aff:a880";
  uiplib_ipaddrconv(server, &ipaddr);
#endif
  /* new connection with remote host */
  client_conn = udp_new(&ipaddr, UIP_HTONS(1001), NULL);
  udp_bind(client_conn, UIP_HTONS(1000));

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  etimer_set(&et, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    if(etimer_expired(&et)) {
      ccnl_generation();
      etimer_restart(&et);
    } else if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
