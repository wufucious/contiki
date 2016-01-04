#include "contiki.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include <stdio.h>
#define SECONDS 2
/*-------------------------------------------------*/
PROCESS(practise_process, "my practise1");
AUTOSTART_PROCESSES(&practise_process);
/*-------------------------------------------------*/
PROCESS_THREAD(practise_process, ev, data)
{
	PROCESS_BEGIN();
	static struct etimer et;
	while(1) 
	{
		etimer_set(&et, CLOCK_SECOND*SECONDS);
		
		PROCESS_WAIT_EVENT();

		if(etimer_expired(&et)) {
			printf("The CLOCK_SECOND is %lu\n", CLOCK_SECOND);
			etimer_reset(&et);
		}
	}
	PROCESS_END();
}
