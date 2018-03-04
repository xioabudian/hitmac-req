#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "dev/leds.h"
#include "net/mac/hitmac/hitmac.h"
#include "net/mac/hitmac/app-router.h"


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
#define SCHEDULE_SEND_INTERVAL (PERIOD*CLOCK_SECOND)
#define HITMAC_DOWNLOAD_TYPE 2
/*--------------------tcpip configure---------------------------------*/
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
/*---------------------root receive data from concentrator---------------*/
/*---------------------concentrator receive appdata from root---------------*/
#define UDP_ROOT_SEND_APPDATA_PORT 8765  //udp_new(remote port)
#define UDP_ROOT_RECEIVE_APPDATA_PORT 5678
/*-------------root receive monitor message from concentrator------------*/
#define UDP_ROOT_SEND_MONITOR_PORT 5688
#define UDP_ROOT_RECEIVE_MONITOR_PORT 3001

static uip_ipaddr_t prefix;
static uint8_t prefix_set;
static struct uip_udp_conn *server_conn;
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
AUTOSTART_PROCESSES(&app_root_process,&app_tcpip_process);//
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
	
	static int i=0;
	linkaddr_t dest;
	char buf[20]="AAAAAAAA";
	sprintf(buf, "root data %d", i);
	i++;
	int len =0;
	while(buf[len]!='\0'){
		len++;
	}
	packetbuf_copyfrom(buf,len+1);
		
	dest.u8[1] =addr>>8;
	dest.u8[0] =0xff & addr;
	
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER,&dest);
	NETSTACK_MAC.send(NULL, NULL);
	

	PRINTF("root app send packet:%2x,len:%d\n",dest.u16,len+1);
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

	PRINTF("phrase: %u\n",get_mod_type());

	if(APP_BUF[0]==0x01&&APP_BUF[1]==0x80&&APP_BUF[2]==0x88&&APP_BUF[3]==0x66&&APP_BUF[4]==0x88){
		struct tsch_asn_t loacl_asn;
		loacl_asn.ls4b = 15600;
		hitmac_set_asn(loacl_asn);
		PRINTF("set root asn\n");
	}
	/*only download permit root sending packets*/
	if(get_mod_type()!= HITMAC_DOWNLOAD_TYPE){
		return;
	}

	if(APP_BUF[0]==0x01&&APP_BUF[1]==0x80){
		/*broadcast*/
		root_send(0xffff);
		
	}else if(APP_BUF[0]==0x02&&APP_BUF[1]==0x01){
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

	int len;
	switch(ev) {
  		
  		case PACKET_INPUT:
	  		// printf("root receive from: %x%x,len:%d\n",input_buf.src_addr.u8[0],input_buf.src_addr.u8[1],input_buf.len);
	  		// printf("data:");
	  		// string = input_buf.buf;
	  		// printf("%s\n", string);
	  		leds_toggle(LEDS_RED);	

	  		/*root upload to concentrator*/
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
	  		input_buf.len = 0;
	  		
	    break;

	    default:
	    break;
  	}
}
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

	server_conn = udp_new(NULL, UIP_HTONS(UDP_ROOT_SEND_APPDATA_PORT), NULL);
	if(server_conn == NULL) {
		PROCESS_EXIT();
	}
	udp_bind(server_conn, UIP_HTONS(UDP_ROOT_RECEIVE_APPDATA_PORT));
    /*set concentrator server ipaddr*/
	uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x02);

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

	
	while(1){
		PROCESS_YIELD();
		if(ev ==PACKET_INPUT){
			eventhandler(ev, data);
		}

		
	}
	PROCESS_END();
}