#include "contiki.h"
#include <string.h>
#include <stdio.h>
#include "serial-ubidots.h"
void send_to_ubidots(const char *api, const char *id, uint16_t *val)
{
	uint8_t i;
	unsigned char buf[6];
	
	printf("\r\n%s", api);
	printf("\t%s", id);
	
	for (i=0; i<UBIDOTS_MSG_16B_NUM; i++) {
		snprintf(buf, 6, "%04d", val[i]);
		printf("\t%s", buf);
	}
	
	printf("\r\n");
}
