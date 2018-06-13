#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "dev/leds.h"
#include "dev/cc26xx-uart.h"
#include "node-id.h"
#include "net/mac/hitmac/hitmac.h"
#include "net/mac/hitmac/hitmac-conf.h"
#include "net/mac/hitmac/hitmac-frame.h"
#include "net/mac/hitmac/app-router.h"
/*--------------------include app header file----------------------------*/
#include "common-header.h"

#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#if RPL_WITH_NON_STORING
#include "net/rpl/rpl-ns.h"
#endif /* RPL_WITH_NON_STORING */
#include "net/netstack.h"
#include "dev/slip.h"

#include <stdio.h>
#include <string.h>
/*--------------------app router configure--------------------------*/
#define PERIOD 7
#define HITMAC_DOWNLOAD_TYPE 2
/*--------------------tcpip configure---------------------------------*/
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
/*---------------------root receive data from concentrator---------------*/
/*---------------------concentrator receive appdata from root---------------*/
#define UDP_ROOT_SEND_APPDATA_PORT 8765  //udp_new(remote port)
#define UDP_ROOT_RECEIVE_APPDATA_PORT 5678
/*-------------concentrator receive monitor message from root------------*/
#define UDP_ROOT_SEND_MONITOR_PORT 5688
#define UDP_ROOT_RECEIVE_MONITOR_PORT 3001
/*----------------------select channel ------------------------------------*/
#define APP_SELECT_CHANNEL_INTERVAL (CLOCK_SECOND*HITMAC_TIME_LENGTH) //15minutes:900 
/*extern valiable;channel select */
uint8_t has_select_channel = 0;
process_event_t select_channel_event;
/*define data struct to store usage of channel*/
struct channel_info{
	uint16_t dest_addr;
	uint8_t node_num;
};
struct channel_info channel_array[HITMAC_MAX_CHANNEL_NUMBER+1];

struct unoccupied_info{
	uint8_t channel[HITMAC_MAX_CHANNEL_NUMBER];
	uint8_t len;
};
struct unoccupied_info unoccupied_channel;
/*---------------------------------------------------------------------------*/
enum {
	HITMAC_APP_DATA,
	HITMAC_NET_HITMONITOR,
};
static uip_ipaddr_t prefix;
static uint8_t prefix_set;
static struct uip_udp_conn *server_conn;
static struct uip_udp_conn *server_conn_monitor;
/*concentrator server ipaddr*/
static uip_ipaddr_t server_ipaddr;
static uip_ipaddr_t nodes_ipaddr;

#ifdef UIP_FALLBACK_INTERFACE
extern struct uip_fallback_interface UIP_FALLBACK_INTERFACE;
#endif

uint8_t APP_BUF[150];

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"
#define MYPRINTF(...) cc26xx_uart_write_byte(0300);printf(__VA_ARGS__);cc26xx_uart_write_byte(0300);

PROCESS(app_root_process,"app root process");
PROCESS(app_tcpip_process,"app tcpip process");
PROCESS(select_channel_process,"select channel process");
AUTOSTART_PROCESSES(&app_tcpip_process);//select_channel_process,app_root_process,&app_tcpip_process
/*---------------------------------------------------------------------------*/
void
logic_test(uint32_t i);
struct total{
	uint16_t id;
	uint16_t num;
};
struct total node_count[10];
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
void 
hitmac_set_conn_process(struct process *p){
	if(p!=NULL){
		conn_process = p;
	}
}
/*---------------------------------------------------------------------------*/
/*app sender function*/
static void root_send(uint16_t addr)
{
#define MAX_LEN 50
	static int i=0;
	linkaddr_t dest;
	char buf[MAX_LEN]="AAAAAAAA";
	sprintf(buf, "root data %d", i);
	i++;
	int len =0;
	while(buf[len]!='\0'){
		len++;
	}

	packetbuf_copyfrom(buf,len+1);
		
	dest.u8[1] =0xff;
	dest.u8[0] =0xff;
	
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER,&dest);
	NETSTACK_MAC.send(NULL, NULL);
	
	MYPRINTF("root app send packet:%2x,len:%d\n",dest.u16,len+1);
	
}
/*---------------------------------------------------------------------------*/
static void
tcpip_handler_border(void)
{	
  if(uip_newdata()) {

	leds_toggle(LEDS_RED);
	/*broadcast*/
	unsigned char buffered_data_length;
	uint8_t channel;
	radio_value_t val;
	buffered_data_length = (unsigned char) ((char *)uip_appdata)[0];
	memset(APP_BUF,0,sizeof(APP_BUF));
	memcpy(APP_BUF, &uip_appdata[1], buffered_data_length);
	
	
	MYPRINTF("phrase: %u\n",get_mod_type());
	//set asn: 88 66 88 + asn
	if(APP_BUF[0]==0x01&&APP_BUF[1]==0x80&&APP_BUF[2]==0x88&&APP_BUF[3]==0x66&&APP_BUF[4]==0x88){
		struct tsch_asn_t loacl_asn;
		loacl_asn.ls4b = APP_BUF[5]<<8|APP_BUF[6];
		hitmac_set_asn(loacl_asn);		
		MYPRINTF("set root asn %lu\n",loacl_asn.ls4b);
		return;
	}
	//manually set channel:77 77 55 55
	if(APP_BUF[0]==0x01&&APP_BUF[1]==0x80&&APP_BUF[2]==0x77&&APP_BUF[3]==0x77&&APP_BUF[4]==0x55&&APP_BUF[5]==0x55){
		channel = APP_BUF[6];
		if(channel<HITMAC_MAX_CHANNEL){
			//store into flash
			normalbyte_rfchannel_burn(0,channel);
			NETSTACK_RADIO.off();
			NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,channel);
			NETSTACK_RADIO.on();
			NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL,&val);
			MYPRINTF("manual set channel %u\n",val);
		}else{
			MYPRINTF("channel number is greater than %d\n",HITMAC_MAX_CHANNEL);
		}
		return;

	//automatic channel scan:55 55 77 77 77
	}else if(APP_BUF[0]==0x01&&APP_BUF[1]==0x80&&APP_BUF[2]==0x55&&APP_BUF[3]==0x55
		&&APP_BUF[4]==0x77&&APP_BUF[5]==0x77&&APP_BUF[6]==0x77){
		// exit app_root_process
		has_select_channel = 0;
		process_exit(&app_root_process);
		hitmac_off(0);
		//start select_channel_process until app select channel and start app_root_process
		process_start(&select_channel_process,NULL);
		MYPRINTF("automatic channel scan\n");
		return;
	}
	//ask channel:66 66 44 66 66
	if(APP_BUF[0]==0x01&&APP_BUF[1]==0x80&&APP_BUF[2]==0x66&&APP_BUF[3]==0x66
		&&APP_BUF[4]==0x44&&APP_BUF[5]==0x66&&APP_BUF[6]==0x66){
		
		NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL,&val);
		MYPRINTF("get current channel %u\n",val);
		return;
	}

	/*only download permit root sending packets*/
	if(get_mod_type()!= HITMAC_DOWNLOAD_TYPE){
		return;
	}
	//only support hitmac broadcast
	if(APP_BUF[0]==0x01&&APP_BUF[1]==0x80){
		/*broadcast*/
		root_send(0xffff);
	}
	else if(APP_BUF[0]==0x02&&APP_BUF[1]==0x01){
		/*unicast*/
		uint16_t addr = APP_BUF[8]<<8 | APP_BUF[9];
		root_send(addr);
	}
	
  }
}

/*---------------------------------------------------------------------------*/
/*event handler*/
static void
eventhandler(process_event_t ev, process_data_t data)
{

	int len = 0;
	radio_value_t channel;
	switch(ev) {
  		
  		case HITMAC_APP_DATA:
	  		leds_toggle(LEDS_RED);
	  		/*root upload app data to concentrator*/
			uip_ipaddr_copy(&server_conn->ripaddr, &server_ipaddr);
			/*set nodes_ipaddr*/
			uip_ip6addr(&nodes_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, input_buf.src_addr.u16);
						
			/*ipv6 addr 16 Bytes*/
			memset(APP_BUF,0,sizeof(APP_BUF));
			len = 0;
			for(int i=0;i<16;i++){
				APP_BUF[i] = nodes_ipaddr.u8[i];				
			}
			len +=16;
			for(int i=len;i<len+input_buf.len;i++){
				APP_BUF[i] = input_buf.buf[i-len];
			}
			len += input_buf.len;

			uip_udp_packet_send(server_conn, APP_BUF, len);
			uip_create_unspecified(&server_conn->ripaddr);
			/*clear input_buf*/
			/*use slip send packet*/
			// MYPRINTF("app:%d\n",input_buf.len);
			/*@test*/
			int flag = 0;
			for(int i=0;i<10;i++){
				if(node_count[i].id==input_buf.src_addr.u16){
	  				node_count[i].num++;
	  				flag =1;
	  				break;
	  			}
			}
			if(flag==0){
				for(int i=0;i<10;i++){
		  			if(node_count[i].id==0){
		  				node_count[i].id = input_buf.src_addr.u16;
		  				break;
		  			}
	  			}
			}
	  		
	  		for(int i=0;i<10;i++){
	  			if(node_count[i].id == input_buf.src_addr.u16){
	  				uint8_t a = node_count[i].id&0xFF;
	  				uint8_t b = (node_count[i].id>>8)&0xFF;
	  				MYPRINTF("id %x%x,count %d,",a,b,node_count[i].num);
					cc26xx_uart_write_byte(0300);
					for(int i =0;i<input_buf.len;i++){
						printf("%c",input_buf.buf[i]);
					}
					printf("\n");
					cc26xx_uart_write_byte(0300);
					MYPRINTF("rssi: %d\n",input_buf.rssi);
	  				break;
	  			}
	  		}

	  		input_buf.len = 0; 


	  		
	    break;

	    case HITMAC_NET_HITMONITOR:
	    	/*root upload net monitor data to concentrator*/
			uip_ipaddr_copy(&server_conn_monitor->ripaddr, &server_ipaddr);
			/*set nodes_ipaddr*/
			uip_ip6addr(&nodes_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, input_buf.src_addr.u16);
			/*ipv6 addr 16 Bytes*/
			memset(APP_BUF,0,sizeof(APP_BUF));
			len = 0;
			for(int i=0;i<16;i++){
				APP_BUF[i] = nodes_ipaddr.u8[i];				
			}
			len +=16;
			//change to rssi
			input_buf.buf[INDEX_RTMETRIC] = input_buf.rssi;
			//change to channel number 
			NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL,&channel);//get local channel
			input_buf.buf[INDEX_TIME_DIFF] = channel;
			//parent
			input_buf.buf[INDEX_PARENT] = input_buf.dest_addr.u8[0];
  			input_buf.buf[INDEX_PARENT+1] = input_buf.dest_addr.u8[1];

			for(int i=len;i<len+input_buf.len;i++){
				APP_BUF[i] = input_buf.buf[i-len];
			}
			len += input_buf.len;

			uip_udp_packet_send(server_conn_monitor, APP_BUF, len);
			uip_create_unspecified(&server_conn_monitor->ripaddr);
			/*clear input_buf*/
			/*use slip send packet*/
			MYPRINTF("monitor:%d\n",input_buf.len);
			
	  		input_buf.len = 0; 		
	    break;

	    default:
	    break;
  	}
}
// /*---------------------------------------------------------------------------*/
// /*net monitor data sender function*/
// void nodes_appdata_send()
// {
// 	uint8_t msg[SYSTEM_MONITOR_MSG_LENGTH];
// 	get_system_monitor_msg(msg,SYSTEM_MONITOR_MSG_LENGTH);

// 	packetbuf_clear();
// 	packetbuf_copyfrom(msg,SYSTEM_MONITOR_MSG_LENGTH);
// 	packetbuf_set_datalen(SYSTEM_MONITOR_MSG_LENGTH);
	
// 	input_buf.len = packetbuf_datalen();
// 	memcpy(input_buf.buf,&msg, input_buf.len);
// 	static int i =0;
// 	i++;
// 	input_buf.src_addr.u16 = i;//0xFE23;
// 	input_buf.dest_addr.u16 = 0xFEA8;

// 	eventhandler(HITMAC_NET_HITMONITOR, NULL);

// }
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTA("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINTA(" ");
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTA("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
void
request_prefix(void)
{
  /* mess up uip_buf with a dirty request... */
  uip_buf[0] = '?';
  uip_buf[1] = 'P';
  uip_len = 2;
  slip_send();
  uip_clear_buf();
}
/*---------------------------------------------------------------------------*/
void
set_prefix_64(uip_ipaddr_t *prefix_64)
{
  rpl_dag_t *dag;
  uip_ipaddr_t ipaddr;
  memcpy(&prefix, prefix_64, 16);
  memcpy(&ipaddr, prefix_64, 16);
  prefix_set = 1;

  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &ipaddr);
  if(dag != NULL) {
    rpl_set_prefix(dag, &prefix, 64);
    MYPRINTF("created a new RPL dag\n");
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_tcpip_process,ev,data)
{
	PROCESS_BEGIN();
	static struct etimer et;
	
	/*start tcp/ip*/
	NETSTACK_NETWORK.init();
#if NETSTACK_CONF_WITH_IPV6
	memcpy(&uip_lladdr.addr, &linkaddr_node_addr, sizeof(uip_lladdr.addr));
	queuebuf_init();
	process_start(&tcpip_process, NULL);
#endif /* NETSTACK_CONF_WITH_IPV6 */
	/* Request prefix until it has been received */
	while(!prefix_set) {
		etimer_set(&et, CLOCK_SECOND);
		request_prefix();
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	}

#if DEBUG || 1
	print_local_addresses();
#endif
/*---------------------concentrator receive appdata from root---------------*/
	server_conn = udp_new(NULL, UIP_HTONS(UDP_ROOT_SEND_APPDATA_PORT), NULL);
	if(server_conn == NULL) {
		PROCESS_EXIT();
	}
	udp_bind(server_conn, UIP_HTONS(UDP_ROOT_RECEIVE_APPDATA_PORT));
/*-------------concentrator receive monitor message from root------------*/
	server_conn_monitor = udp_new(NULL, UIP_HTONS(UDP_ROOT_SEND_MONITOR_PORT), NULL);
	if(server_conn_monitor == NULL) {
		PROCESS_EXIT();
	}
	udp_bind(server_conn_monitor, UIP_HTONS(UDP_ROOT_RECEIVE_MONITOR_PORT));

	
    /*set concentrator server ipaddr*/
	uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x02);

	process_start(&app_root_process,NULL);

	

	while(1){
		PROCESS_YIELD();
		if(ev == tcpip_event) {
			/*concentrator download to root*/
			tcpip_handler_border();
		}
		
		
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(select_channel_process,ev,data)
{
	PROCESS_BEGIN();
	static struct etimer ch_et;
	static radio_value_t ch_num = RF_CORE_CONF_CHANNEL;
	int ch;
	radio_value_t channel;

	etimer_set(&ch_et,APP_SELECT_CHANNEL_INTERVAL);
	NETSTACK_RADIO.on();
	select_channel_event = process_alloc_event();
	memset(channel_array,0,sizeof(channel_array));
	memset(unoccupied_channel.channel,0,sizeof(unoccupied_channel.channel));
	unoccupied_channel.len =0;
	
	while(1){
		PROCESS_YIELD();
		if(etimer_expired(&ch_et))
		{
			NETSTACK_RADIO.off();
			ch_num++;			
			NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,ch_num%HITMAC_MAX_CHANNEL_NUMBER);
			
			NETSTACK_RADIO.on();

			NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL,&channel);//get local channel
			MYPRINTF("SET SCAN CHANNEL %d\n",channel);

			if(ch_num>=(HITMAC_MAX_CHANNEL_NUMBER+RF_CORE_CONF_CHANNEL)){//scan process end
				ch_num = RF_CORE_CONF_CHANNEL ;
				has_select_channel =1;

				for(ch=0;ch<HITMAC_MAX_CHANNEL_NUMBER;ch++){
					if(channel_array[ch].node_num==0&&unoccupied_channel.len<HITMAC_MAX_CHANNEL_NUMBER){//the channel has not been occupied
						unoccupied_channel.channel[unoccupied_channel.len] = ch;
						unoccupied_channel.len++;
						MYPRINTF("not occupied channel %d\n",ch);
					}else{
						MYPRINTF("occupied channel %d,nodes number %d\n",ch,channel_array[ch].node_num);
					}
				}

				/*select channel from unoccupied channel list*/
				NETSTACK_RADIO.off();
				if(unoccupied_channel.len!=0){
					ch = random_rand()%unoccupied_channel.len;
					NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,unoccupied_channel.channel[ch]);
				}else{
					NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,random_rand()%HITMAC_MAX_CHANNEL_NUMBER);
				}
				NETSTACK_RADIO.on();
				NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL,&channel);//get local channel
				MYPRINTF("RANDOM SET ROOT CHANNEL %d\n",channel);

				normalbyte_rfchannel_burn(0,channel);
				NETSTACK_RADIO.off();
				NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,channel);
				NETSTACK_RADIO.on();

				hitmac_off(1);//hitmac process has been started before.
				process_start(&app_root_process,NULL);
				
				MYPRINTF("scan end\n");
				PROCESS_EXIT();
			}

			if(!has_select_channel){
				etimer_set(&ch_et,APP_SELECT_CHANNEL_INTERVAL);
			}
			
		}
		if(ev == select_channel_event){
			NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL,&channel);//get local channel
			MYPRINTF("app listen current channel %d\n",channel);
			

			if(input_buf.type==FRAME802154_DATAFRAME&&
				linkaddr_cmp(&input_buf.dest_addr, &linkaddr_null) == 0){
				
				channel_array[channel].node_num++;
				channel_array[channel].dest_addr=packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u16;

				MYPRINTF("packet attrutute:%d\n",input_buf.type);
				MYPRINTF("sender addr:%x\n",input_buf.src_addr.u16);
				MYPRINTF("receive addr:%x\n",input_buf.dest_addr.u16);
				MYPRINTF("receive rssi:%d\n",input_buf.rssi);
				MYPRINTF("data len:%d\n",input_buf.len);

			}

		    input_buf.len = 0;
				
		}


	}

	PROCESS_END();

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_root_process,ev,data)
{
	PROCESS_BEGIN();
	
	hitmac_is_root = 1;
	NETSTACK_RADIO.on();
	NETSTACK_CONF_MAC.on();

	hitmac_set_conn_process(&app_root_process);
	MYPRINTF("app root started\n");
	radio_value_t val;
	has_select_channel =1; 
	/*@test upload*/
	// uint8_t test_buf[20]="hello world\n";
	// static struct etimer et;
	// etimer_set(&et,CLOCK_SECOND*2);
	// static uint8_t i=0;

	NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL,&val);//get local channel
	MYPRINTF("app root channel %d\n",val);

	/*@test count*/
	for(int i=0;i<10;i++){
		node_count[i].id=0;
		node_count[i].num=0;
	}


	while(1){
		PROCESS_YIELD();
		if(ev == PACKET_INPUT){
			if(input_buf.buf[0]=='A'){//hello world
				eventhandler(HITMAC_APP_DATA, data);
			}else if(input_buf.buf[0]=='B'){
				eventhandler(HITMAC_NET_HITMONITOR, data);
			}
			
			
		}

		/*@test upload*/
		// if(etimer_expired(&et)){
		// 	/*@test upload*/
		// 	// packetbuf_clear();
		// 	// packetbuf_copyfrom(test_buf,20);
		// 	// packetbuf_set_datalen(20);
			
		// 	// input_buf.len = packetbuf_datalen();
		// 	// memcpy(input_buf.buf,&test_buf, input_buf.len);
		// 	// input_buf.src_addr.u16 = 0xFE23;
		// 	// input_buf.dest_addr.u16 = 0xFEA8;

		// 	// eventhandler(HITMAC_NET_HITMONITOR, data);
		// 	nodes_appdata_send();
		// 	etimer_set(&et,CLOCK_SECOND*5);
		// }
		/*download test*/
		// if(etimer_expired(&et)){

		// 	if(get_mod_type()== HITMAC_DOWNLOAD_TYPE){
		// 		root_send(0xffff);
		// 	}
		// 	etimer_set(&et,CLOCK_SECOND*PERIOD);
		// }
		/*@test mac on or off test*/
		// if(etimer_expired(&et)){
		// 	hitmac_off(i%2);
		// 	i++;
		// 	leds_toggle(LEDS_RED);
		// 	etimer_set(&et,CLOCK_SECOND*20);
		// }
		
	}
	PROCESS_END();
}