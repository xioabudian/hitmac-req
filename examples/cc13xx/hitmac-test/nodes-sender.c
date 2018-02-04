#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "dev/leds.h"
#include "net/mac/hitmac/hitmac.h"

#include <stdio.h>
#include <string.h>

#define PERIOD 15
#define SEND_INTERVAL (PERIOD*CLOCK_SECOND)

PROCESS(nodes_sender_process,"nodes sender process");
AUTOSTART_PROCESSES(&nodes_sender_process);

PROCESS_THREAD(nodes_sender_process,ev,data)
{
	PROCESS_BEGIN();
	static struct etimer periodic;
	etimer_set(&periodic,SEND_INTERVAL);

	hitmac_is_root = 0;
	printf("nodes sender\n");
	NETSTACK_CONF_MAC.on();
	while(1){
		PROCESS_YIELD_UNTIL(ev==PROCESS_EVENT_TIMER);
		if(etimer_expired(&periodic)){
			
			
			// NETSTACK_CONF_MAC.send(NULL,NULL);
		}
	}
	PROCESS_END();
}