#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "dev/leds.h"
#include "net/mac/hitmac/hitmac.h"
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
#define PERIOD 10
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

PROCESS(app_root_process,"app root process");
PROCESS(app_tcpip_process,"app tcpip process");
AUTOSTART_PROCESSES(&app_tcpip_process);//,app_root_process,&app_tcpip_process
/*---------------------------------------------------------------------------*/
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
	
	PRINTF("%c",0300);
	PRINTF("root app send packet:%2x,len:%d\n",dest.u16,len+1);
	PRINTF("%c",0300);
}
/*---------------------------------------------------------------------------*/
static void
tcpip_handler_border(void)
{	
  if(uip_newdata()) {

	leds_toggle(LEDS_RED);
	/*broadcast*/
	unsigned char buffered_data_length;
	buffered_data_length = (unsigned char) ((char *)uip_appdata)[0];
	memset(APP_BUF,0,sizeof(APP_BUF));
	memcpy(APP_BUF, &uip_appdata[1], buffered_data_length);
	printf("%c",0300);
	printf("phrase: %u\n",get_mod_type());
	printf("%c",0300);
	if(APP_BUF[0]==0x01&&APP_BUF[1]==0x80&&APP_BUF[2]==0x88&&APP_BUF[3]==0x66&&APP_BUF[4]==0x88){
		struct tsch_asn_t loacl_asn;
		loacl_asn.ls4b = APP_BUF[5]<<8|APP_BUF[6];
		hitmac_set_asn(loacl_asn);
		printf("%c",0300);
		printf("set root asn %lu\n",loacl_asn.ls4b);
		printf("%c",0300);
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
	switch(ev) {
  		
  		case HITMAC_APP_DATA:
	  		// printf("root receive from: %x%x,len:%d\n",input_buf.src_addr.u8[0],input_buf.src_addr.u8[1],input_buf.len);
	  		// printf("data:");
	  		// string = input_buf.buf;
	  		// printf("%s\n", string);
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
			// printf("%c",0300);
			// printf("app:%d\n",input_buf.len);
			// printf("%c",0300);
			
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
			// printf("%c",0300);
			// printf("monitor:%d\n",input_buf.len);
			// printf("%c",0300);
			
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
// 	input_buf.src_addr.u16 = 0xFE23;
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
    PRINTF("created a new RPL dag\n");
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
PROCESS_THREAD(app_root_process,ev,data)
{
	PROCESS_BEGIN();
	
	hitmac_is_root = 1;
	
	NETSTACK_CONF_MAC.off(1);
	NETSTACK_CONF_MAC.on();

	hitmac_set_conn_process(&app_root_process);
	printf("app root started\n");
	
	/*@test upload*/
	// uint8_t test_buf[20]="hello world\n";
	// static struct etimer et;
	// etimer_set(&et,CLOCK_SECOND*2);

	while(1){
		PROCESS_YIELD();
		if(ev == PACKET_INPUT){
			if(input_buf.buf[0]=='A'){//hello world
				eventhandler(HITMAC_APP_DATA, data);
			}else if(input_buf.buf[0]=='B'){
				eventhandler(HITMAC_NET_HITMONITOR, data);
			}
			
			
		}

		// /*@test upload*/
		// if(etimer_expired(&et)){
		// 	/*@test upload*/
		// 	packetbuf_clear();
		// 	packetbuf_copyfrom(test_buf,20);
		// 	packetbuf_set_datalen(20);
			
		// 	input_buf.len = packetbuf_datalen();
		// 	memcpy(input_buf.buf,&test_buf, input_buf.len);
		// 	input_buf.src_addr.u16 = 0xFE23;
		// 	input_buf.dest_addr.u16 = 0xFEA8;

		// 	eventhandler(HITMAC_NET_HITMONITOR, data);
		// 	// nodes_appdata_send();
		// 	etimer_set(&et,CLOCK_SECOND*5);
		// }
		/*download test*/
		// if(etimer_expired(&et)){

		// 	if(get_mod_type()== HITMAC_DOWNLOAD_TYPE){
		// 		root_send(0xffff);
		// 	}
		// 	etimer_set(&et,CLOCK_SECOND*2);
		// }

		
	}
	PROCESS_END();
}