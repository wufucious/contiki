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

#include "leds.h"

#include <string.h>
#include <stdbool.h>

#undef DEBUG
#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

//#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_IP_BUF   ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#include "ccn-lite-contiki.h"

#define GENERATE_INTERVAL		MOTE3_INTERVAL//5 * CLOCK_SECOND
#define MAX_PREFIX_SIZE		40
#define CCN_BUF_SIZE    (128)

static char prefix_buf[MAX_PREFIX_SIZE];
static unsigned char ccn_content_buf[CCN_BUF_SIZE];


static struct uip_udp_conn *client_conn;

/*---------------------------------------------------------------------------*/
PROCESS(ccn_client_process, "ccn client mote2 process");
AUTOSTART_PROCESSES(&ccn_client_process);
/*---------------------------------------------------------------------------*/
static void
fade(unsigned char l)
{
  volatile int i;
  int k, j;
  for(k = 0; k < 800; ++k) {
    j = k > 400 ? 800 - k : k;

    leds_on(l);
    for(i = 0; i < j; ++i) {
      __asm("nop");
    }
    leds_off(l);
    for(i = 0; i < 400 - j; ++i) {
      __asm("nop");
    }
  }
}
/*---------------------------------------------------------------------------*/
#define TRANS_BUF_SIZE 150
static char buf_trans[TRANS_BUF_SIZE];

static void
tcpip_handler(void)
{
  char *str;
  int len = 0;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    PRINTF("Got interests from the mote1: '%s'\n trying to translate that \n", str);
    int suite = CCNL_SUITE_NDNTLV;
//    int suite = CCNL_SUITE_CCNTLV;
    memset(buf_trans, '\0', TRANS_BUF_SIZE);
    int k = ccnl_find_content(suite, uip_appdata, uip_datalen(), buf_trans, &len);
    if(k == 0){
    	PRINTF("sending out match content");
    	fade(LEDS_GREEN);

    	uip_ipaddr_copy(&client_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
    	client_conn->rport = UIP_IP_BUF->srcport;

      	uip_udp_packet_send(client_conn, buf_trans, len);

      	memset(&client_conn->ripaddr, 0, sizeof(client_conn->ripaddr));
      	client_conn->rport=0;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
ccnl_generation(void)
{
  static int seq_id;
  if(seq_id == SEQ_ID_MAX) seq_id=0;

  PRINTF("mote2 generating new content: ");
  sprintf(prefix_buf, "kista/kth/sics/alice/%d", ++seq_id);
  PRINTF(" (msg: %s)\n", prefix_buf);

  int suite = CCNL_SUITE_NDNTLV;
//  int suite = CCNL_SUITE_CCNTLV;
  int offs;
//  unsigned char *data;

  memset(ccn_content_buf, '\0', CCN_BUF_SIZE);
  char* content="a\nb\0c";
  int conlen = 5;

  offs = ccnl_cache_content(suite, prefix_buf, content, conlen);

  if(offs == -1){
  	PRINTF("failed to make content!\n");
  }else
  {
  	PRINTF("content generated!\n");
  	fade(LEDS_RED);
  }

  //data = ccn_content_buf + offs;
  //uip_udp_packet_send(client_conn, data, len);
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
PROCESS_THREAD(ccn_client_process, ev, data)
{
  static struct etimer et;
//  uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();
  PRINTF("ccn client process started\n");

#if UIP_CONF_ROUTER
  set_global_address();
#endif

  print_local_addresses();

  client_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(client_conn, UIP_HTONS(1001));

  PRINTF("Waiting for the Interest from IP address ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  ccnl_init();

  etimer_set(&et, GENERATE_INTERVAL);
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
