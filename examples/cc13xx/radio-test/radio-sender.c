#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "bat-voltage.h"
#include "lib/random.h"
#include "dev/leds.h"
// #include "cc1310-pa-test.h"
#include "node-id.h"

#include <stdio.h>
#include <string.h>
#define PERIOD 2//
#define SEND_INTERVAL (15*CLOCK_SECOND/2)
#define PA_LISTEN_ACK_WAIT_TIME (RTIMER_SECOND/5)
#define BUSYWAIT_UNTIL_ABS(cond, t0, offset) while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), (t0) + (offset))) ;
// static int i=0;
void logic_test(uint32_t i);
PROCESS(radio_sender_process,"radio sender process");
AUTOSTART_PROCESSES(&radio_sender_process);
PROCESS_THREAD(radio_sender_process,ev,data)
{
	
	static struct etimer periodic;
	PROCESS_BEGIN();
 #define APP_PAYLOAD 75
    printf("%s\n","Startting sender");
	etimer_set(&periodic,SEND_INTERVAL);
    static char buf[APP_PAYLOAD]="AAAAAAAA";
    char *sender=buf;

    #define ACK_LEN 3
	uint8_t ackbuf[ACK_LEN] = {0,0,0};
	int ack_len;
	int is_packet_pending = 0;
    //add node's rssi
  	static radio_value_t rssi=-50;
  	static int i=0;
  	static int j=0;
  	int len;
	while(1){
		PROCESS_YIELD_UNTIL(ev==PROCESS_EVENT_TIMER);
		if(etimer_expired(&periodic)){
			etimer_set(&periodic,random_rand()%SEND_INTERVAL+3*CLOCK_SECOND);

			{
				NETSTACK_RADIO.on();
	  			// NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI,&rssi);//get local rssi
				// len = sprintf(buf, "rssi %d", rssi);
				// len += sprintf(buf +len, ",Hello %d,", i);
				// len += sprintf(buf +len, "%d", j);
				len = sprintf(buf, "nodeid %x", node_id);
				len += sprintf(buf +len, ",Hello %d,", i);
				len += sprintf(buf +len, "%d\n", j);
				buf[APP_PAYLOAD-1] = '\0';
				
				logic_test(1);
			    NETSTACK_RADIO.send(buf,APP_PAYLOAD);
			    logic_test(0);
			   /* We are not coordinator, try to wait ack */
			    rtimer_clock_t t0;
			    t0 = RTIMER_NOW();
			    /*Wait  for one timslots for ACK*/
			    

			    BUSYWAIT_UNTIL_ABS((is_packet_pending = NETSTACK_RADIO.pending_packet()), t0,PA_LISTEN_ACK_WAIT_TIME);
				if(is_packet_pending){
					ack_len = NETSTACK_RADIO.read(ackbuf,ACK_LEN);
					if(ack_len==ACK_LEN){
						j++;
					}
				}
				i ++;
				leds_toggle(LEDS_RED);
				
				NETSTACK_RADIO.off();
			}
			
			
			
		}
	}
	
	PROCESS_END();

	
}