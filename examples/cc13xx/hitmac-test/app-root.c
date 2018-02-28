#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "dev/leds.h"
#include "net/mac/hitmac/hitmac.h"
#include "net/mac/hitmac/app-router.h"


#include <stdio.h>
#include <string.h>

#define PERIOD 60
#define SEND_INTERVAL (PERIOD*CLOCK_SECOND)
#define HITMAC_DOWNLOAD_TYPE 2

PROCESS(app_root_process,"app root process");
AUTOSTART_PROCESSES(&app_root_process);

void
logic_test(uint32_t i);
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
	  		printf("root receive from: %x%x,len:%d\n",input_buf.src_addr.u8[0],input_buf.src_addr.u8[1],input_buf.len);
	  		printf("data:");
	  		string = input_buf.buf;
	  		printf("%s\n", string);
	  		leds_toggle(LEDS_RED);
	  		/*clear input_buf*/
	  		input_buf.len = 0;
	  		
	    break;

	    default:
	    break;
  	}
}
/*---------------------------------------------------------------------------*/
/*app sender function*/
static void root_send()
{
	static int i=0;
	linkaddr_t dest;
	dest.u8[1] =0x7e;
	dest.u8[0] =0x1c;
	dest.u8[1] =0xff;
	dest.u8[0] =0xff;

	char buf[20]="AAAAAAAA";
	sprintf(buf, "Hello %d", i);
	i++;
	int len =0;
	while(buf[len]!='\0'){
		len++;
	}
	packetbuf_copyfrom(buf,len+1);

	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER,&dest);

	leds_toggle(LEDS_RED);

	NETSTACK_MAC.send(NULL, NULL);
	printf("root app send packet:%s,len:%d\n",buf,len+1);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_root_process,ev,data)
{
	PROCESS_BEGIN();
	static struct etimer periodic;
	etimer_set(&periodic,SEND_INTERVAL);

	hitmac_is_root = 1;
	
	NETSTACK_CONF_MAC.off(1);
	NETSTACK_CONF_MAC.on();

	hitmac_set_conn_process(&app_root_process);

	printf("app root\n");
	while(1){
		PROCESS_YIELD();
		if(ev ==PACKET_INPUT){
			eventhandler(ev, data);
		}

		if(etimer_expired(&periodic)){
			/*only download permit root sending packets*/
			if(get_mod_type()== HITMAC_DOWNLOAD_TYPE){
				root_send();
			}
			etimer_set(&periodic,SEND_INTERVAL);
		}
	}
	PROCESS_END();
}