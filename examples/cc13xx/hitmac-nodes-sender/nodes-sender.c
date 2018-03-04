#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "dev/leds.h"
#include "net/mac/hitmac/hitmac.h"
#include "net/mac/hitmac/app-router.h"

#include <stdio.h>
#include <string.h>

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

#define PERIOD 60*15//60*15
#define SEND_INTERVAL (PERIOD*CLOCK_SECOND)

PROCESS(nodes_sender_process,"nodes sender process");
AUTOSTART_PROCESSES(&nodes_sender_process);

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
	  		printf("nodes receive from: %x%x,len:%d\n",input_buf.src_addr.u8[0],input_buf.src_addr.u8[1],input_buf.len);
	  		printf("data:");
	  		string = input_buf.buf;
	  		printf("%s\n", string);
	  		/*clear input_buf*/
	  		input_buf.len = 0;
	  		leds_toggle(LEDS_RED);
	  		
	    break;

	    default:
	    break;
  	}
}
/*---------------------------------------------------------------------------*/
/*mac return status*/
static void
packet_sent(void *ptr, int status, int transmissions)
{
  PRINTF("mac_callback_t %p ptr %p status %d num_tx %d\n",
         (void *)sent, ptr, status, num_tx);
  switch(status) {
  case MAC_TX_COLLISION:
    PRINTF("mac: collision after %d tx\n", num_tx);
    break; 
  case MAC_TX_NOACK:
    PRINTF("mac: noack after %d tx\n", num_tx);
    break;
  case MAC_TX_OK:
    PRINTF("mac: sent after %d tx\n", num_tx);
    break;
  default:
    PRINTF("mac: error %d after %d tx\n", status, num_tx);
  }
  
}
/*---------------------------------------------------------------------------*/
/*app sender function*/
static void nodes_send()
{
	static int i=0;
	char buf[20]="AAAAAAAA";
	sprintf(buf, "Hello %d", i);
	i++;
	int len =0;
	while(buf[len]!='\0'){
		len++;
	}
	packetbuf_copyfrom(buf,len+1);

	NETSTACK_MAC.send(&packet_sent, NULL);
}
PROCESS_THREAD(nodes_sender_process,ev,data)
{
	PROCESS_BEGIN();
	static struct etimer periodic;
	etimer_set(&periodic,CLOCK_SECOND * 2);

	hitmac_is_root = 0;
	printf("nodes sender\n");
	NETSTACK_CONF_MAC.on();

	hitmac_set_conn_process(&nodes_sender_process);
	while(1){
		PROCESS_YIELD();
		if(etimer_expired(&periodic)){
			nodes_send();
			printf("nodes app send packet");
			etimer_set(&periodic,SEND_INTERVAL);
		}
		if(ev ==PACKET_INPUT){
			eventhandler(ev, data);
		}

	}
	PROCESS_END();
}