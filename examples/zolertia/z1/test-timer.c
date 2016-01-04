#include "contiki.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include <stdio.h>
#define SECONDS 2
/*-------------------------------------------------*/
PROCESS(hello_timer_process, "hello world with timer example");
AUTOSTART_PROCESSES(&hello_timer_process);
/*-------------------------------------------------*/
PROCESS_THREAD(hello_timer_process, ev, data)
{
	PROCESS_BEGIN();
	static struct etimer et;
	while(1) 
	{
		etimer_set(&et, CLOCK_SECOND*SECONDS);
		
		PROCESS_WAIT_EVENT();

		if(etimer_expired(&et)) {
			printf("Hello world!\n");
			etimer_reset(&et);
		}
	}
	PROCESS_END();
}
