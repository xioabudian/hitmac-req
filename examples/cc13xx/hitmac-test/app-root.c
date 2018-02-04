#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "dev/leds.h"
#include "net/mac/hitmac/hitmac.h"

#include <stdio.h>
#include <string.h>

#define PERIOD 10
#define SEND_INTERVAL (PERIOD*CLOCK_SECOND)

PROCESS(app_root_process,"app root process");
AUTOSTART_PROCESSES(&app_root_process);

PROCESS_THREAD(app_root_process,ev,data)
{
	PROCESS_BEGIN();
	static struct etimer periodic;
	etimer_set(&periodic,SEND_INTERVAL);

	hitmac_is_root = 1;
	
	NETSTACK_CONF_MAC.off(1);
	NETSTACK_CONF_MAC.on();

	printf("app root\n");
	while(1){
		PROCESS_YIELD_UNTIL(ev==PROCESS_EVENT_TIMER);
		if(etimer_expired(&periodic)){
			
		}
	}
	PROCESS_END();
}