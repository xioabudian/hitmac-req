#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "bat-voltage.h"
#include "lib/random.h"
#include "dev/leds.h"

#include <stdio.h>
#include <string.h>
#define PERIOD 2
#define SEND_INTERVAL (PERIOD*CLOCK_SECOND)
// static int i=0;
void logic_test(uint32_t i);
PROCESS(radio_sender_process,"radio sender process");
AUTOSTART_PROCESSES(&radio_sender_process);
PROCESS_THREAD(radio_sender_process,ev,data)
{
	
	static struct etimer periodic;
	PROCESS_BEGIN();
    
    printf("%s\n","Startting sender");
	etimer_set(&periodic,SEND_INTERVAL);
    static char buf[20]="AAAAAAAA";
    char *sender=buf;
    //add node's rssi
  	static radio_value_t rssi=-50;
	while(1){
		PROCESS_YIELD_UNTIL(ev==PROCESS_EVENT_TIMER);
		if(etimer_expired(&periodic)){
			// i = get_voltage();
			// add node's rssi
			// NETSTACK_RADIO.on();
  			NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI,&rssi);//get local rssi
			// i ++;
			rssi = 5;
			sprintf(buf, "Hello %d,", rssi);
			// logic_test(1);
			printf("send len:%d ",sizeof(buf));
		    NETSTACK_RADIO.send(buf,sizeof(buf));
		    // NETSTACK_RADIO.off();
		    // logic_test(0);
			// printf("send %s\n",sender);
			leds_toggle(LEDS_RED);
			etimer_reset(&periodic);
		}
	}
	
	PROCESS_END();

	// PROCESS_BEGIN();
	// static struct etimer periodic;
	// uint32_t interval = SEND_INTERVAL;
	// etimer_set(&periodic,interval);
	// char buf[25]="Hello World!";
	// char ack_buf[7];
	// char *sender = ack_buf;
	// while(1){
	// 	PROCESS_YIELD_UNTIL(ev==PROCESS_EVENT_TIMER);

	// 	if(etimer_expired(&periodic)){
			
	// 		uint32_t interval = SEND_INTERVAL + random_rand()%(CLOCK_SECOND/2);
	// 		etimer_set(&periodic,interval);

	// 		if(NETSTACK_RADIO.channel_clear()==0){
	// 			printf("sending collision\n");
	// 			continue;
	// 		}else{
	// 			i ++;
	// 			sprintf(buf, "Hello %d,", i);
	// 			NETSTACK_RADIO.send(buf,sizeof(buf));

	// 			//delay

	// 			NETSTACK_RADIO.read(ack_buf,5);
	// 			printf("TX_OK %d %s\n",i,sender);

				
	// 		}

			
	// 	}
	// }
	// PROCESS_END();
}