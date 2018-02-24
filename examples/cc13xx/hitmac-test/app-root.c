#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "dev/leds.h"
#include "net/mac/hitmac/hitmac.h"
#include "net/mac/hitmac/app-router.h"

#include <stdio.h>
#include <string.h>

#define PERIOD 10
#define SEND_INTERVAL (PERIOD*CLOCK_SECOND)

PROCESS(app_root_process,"app root process");
AUTOSTART_PROCESSES(&app_root_process);

int temptemp = 3;
/*---------------------------------------------------------------------------*/
void 
hitmac_set_conn_process(struct process *p){
	if(p!=NULL){
		conn_process = p;
	}
}
/*---------------------------------------------------------------------------*/
/*event handler*/
static void
eventhandler(process_event_t ev, process_data_t data)
{
	uint8_t *string;
	switch(ev) {
  		
  		case PACKET_INPUT:
	  		printf("app receive from: %x%x,len:%d\n",input_buf.src_addr.u8[0],input_buf.src_addr.u8[1],input_buf.len);
	  		printf("data:");
	  		string = input_buf.buf;
	  		printf("%s\n", string);
	  		/*clear input_buf*/
	  		input_buf.len = 0;
	  		
	    break;

	    default:
	    break;
  	}
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_root_process,ev,data)
{
	PROCESS_BEGIN();
	// static struct etimer periodic;
	// etimer_set(&periodic,SEND_INTERVAL);
	hitmac_is_root = 1;
	
	NETSTACK_CONF_MAC.off(1);
	NETSTACK_CONF_MAC.on();

	hitmac_set_conn_process(&app_root_process);

	printf("app root\n");
	while(1){
		PROCESS_YIELD();
    	eventhandler(ev, data);
	}
	PROCESS_END();
}