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

#undef DEBUG
#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

//#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_IP_BUF   ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#include "ccn-lite-contiki.h"

#define GENERATE_INTERVAL		10 * CLOCK_SECOND//5 * CLOCK_SECOND
#define MAX_PAYLOAD_LEN		40

static struct uip_udp_conn *client_conn;

#define PRE_TRY_CONNCETION 0
#define ENABLE_CCNL

//#define CFS_DEBUG

//#define CCNL_MAKE_INTEREST
#define CCNL_TRANS

//#define CPU_TEMP
#ifdef CPU_TEMP
#include "lib/sensors.h"
#include "dev/adc-sensors.h"
#include "dev/cpu-temp-sensor.h"
#include "adc.h"

const struct sensors_sensor cputemp_sensor;

static int active;
static int last_mv_value, last_celsius_value;

static int value(int type)
{
  int value;
  /*NOTE: "type" could refine the output to
   * RAW digital value or some physical unit (lux, etc) */

  switch(type){

    case CPUTEMPSENSOR_MV:
      value = adc_measure_channel(ADC_CHANNEL_INTERNAL_TEMP);
      last_mv_value = value;
      break;

    case CPUTEMPSENSOR_CELSIUS:
      value = adc_measure_int_temp();
      last_celsius_value = value;
      break;

    case CPUTEMPSENSOR_MV_LAST:
      value = last_mv_value;
      break;

    case CPUTEMPSENSOR_CELSIUS_LAST:
      value = last_celsius_value;
      break;

    default:
      value = adc_measure_int_temp();
      break;
  }

  return value;
}

/*---------------------------------------------------------------------------*/

static int status(int type)
{
  return active;
}

/*---------------------------------------------------------------------------*/

static int configure(int type, int c)
{
  switch (type) {

    case SENSORS_HW_INIT:
      /* init ADC */
      adc_sensors_init();

      active = c;
      if(active){
        ;
      }
      else{
        /* Power OFF the sensors for now - better power management !?? */
        ;
      }
      break;

    case SENSORS_ACTIVE:
      active  = c;
      if (active)
        ;
      else
        ;
      break;

    case SENSORS_READY:
      return active;  // return value
      break;

    default:
      return 1;  // error
  }
  return 0;
}

/*---------------------------------------------------------------------------*/

SENSORS_SENSOR(cputemp_sensor, "Internal Temperature", value, configure, status);

/*---------------------------------------------------------------------------*/
/* Light Polling Process for generating events in case a threshold is exceeded */
#define ABS(x) (x < 0 ? -x : x)

#endif

/*---------------------------------------------------------------------------*/
PROCESS(ccn_client_process, "ccn client process");
AUTOSTART_PROCESSES(&resolv_process,&ccn_client_process);
/*---------------------------------------------------------------------------*/
#ifdef CCNL_TRANS
#define TRANS_BUF_SIZE 150
static char buf_trans[TRANS_BUF_SIZE];
#endif

static void
tcpip_handler(void)
{
  char *str;
  int len = 0;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
//    printf("Response from the mote1: '%s'\n", str);
#ifdef CCNL_TRANS
    printf("Got interests from the mote1: '%s'\n trying to translate that \n", str);
    int suite = CCNL_SUITE_NDNTLV;
//    int suite = CCNL_SUITE_CCNTLV;
    memset(buf_trans, '\0', TRANS_BUF_SIZE);
    int k = ccnl_find_content(suite, uip_appdata, uip_datalen(), buf_trans, &len);
    if(k == 0){
    	printf("sending out match content");

    	uip_ipaddr_copy(&client_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
    	client_conn->rport = UIP_IP_BUF->srcport;

      	uip_udp_packet_send(client_conn, buf_trans, len);

      	memset(&client_conn->ripaddr, 0, sizeof(client_conn->ripaddr));
      	client_conn->rport=0;
    }
#endif
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
  if(seq_id == SEQ_ID_MAX) seq_id=0;//TODO: debug_level=0 and seq_id=1003 will be freea bug

  printf("mote2 generate new content: ");
//  PRINT6ADDR(&client_conn->ripaddr);
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
    unsigned char *data;
    memset(_int_buf, '\0', BUF_SIZE);
#ifdef CCNL_MAKE_INTEREST
    offs = ccnl_make_interest(suite, buf, NULL,_int_buf, BUF_SIZE, &len);
  	if(offs == -1){
  		printf("failed to make interest!\n");
  	}
  	uip_udp_packet_send(client_conn, _int_buf, strlen(_int_buf));
#else
#ifdef CPU_TEMP
    SENSORS_ACTIVATE(cputemp_sensor);
    int temp = cputemp_sensor.value(CPUTEMPSENSOR_MV);
    SENSORS_DEACTIVATE(cputemp_sensor);
    char content[5];
    sprintf(content,"%d",temp);
    content[4]="\0";
#else
  	char* content="5635\0";
#endif
  	offs = ccnl_make_content(suite, buf, content, NULL,_int_buf, &len);
  	if(offs == -1){
  		printf("failed to make content!\n");
  	}else
  	{
  		printf("content generated!\n");
  	}
//  	data = _int_buf + offs;

//  	uip_udp_packet_send(client_conn, data, len);
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

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
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
    }ccnl_generation
  }
#else
  static char *server="aaaa::d200:0:0:1000";
  uiplib_ipaddrconv(server, &ipaddr);
#endif
  /* new connection with remote host */
//  client_conn = udp_new(&ipaddr, UIP_HTONS(1000), NULL);
  client_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(client_conn, UIP_HTONS(1001));

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

//  ccnl_generation();
  ccnl_init();

  etimer_set(&et, GENERATE_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    if(etimer_expired(&et)) {
//      while(1){
    	  ccnl_generation();
//      }
      etimer_restart(&et);
    } else if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
