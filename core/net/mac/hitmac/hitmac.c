#include "contiki.h"

#include "dev/radio.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/mac/hitmac/hitmac.h"
#include "net/mac/hitmac/hitmac-frame.h"
#include "dev/leds.h"

#include <stdio.h>
#if 0
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else 
#define PRINTF(...)
#define PRINTLLADDR(...)
#endif 

int hitmac_is_root;

int hitmac_is_started;

uint8_t lost_sync_num = 0;

static int hitmac_is_associated;

static struct rtimer asn_rtimer;

static struct hitmac_asn_t hitmac_current_asn;

static struct hitmac_scheduler_t hitmac_current_scheduler;

static uint32_t mod_type;
/*record current asn timeslot start point */
static rtimer_clock_t current_asn_time;
/*root receive request timeoffset*/
rtimer_clock_t receive_request_time;

PT_THREAD(hitmac_request(struct pt *pt));
/*root send eb packet periodically*/
PROCESS(hitmac_root_eb_process,"HITMAC: hitmac root eb process");
/*scheduler process*/
PROCESS(hitmac_scheduler_process,"HITMAC: hitmac scheduler process");
/*The main hitmac process*/
PROCESS(hitmac_process, "HITMAC: main process");
/*---------------------------------------------------------------------------*/
/*@test*/
void
logic_test(uint32_t i);
/*---------------------------------------------------------------------------*/
/*on a timeslot , send packet timeoffset*/
int 
hitmac_send_packet(uint8_t type){

  int len = 0;
  if(type == FRAME802154_BEACONFRAME){
    packetbuf_clear();
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_BEACONFRAME);

    len = hitmac_packet_create_eb(packetbuf_dataptr(), PACKETBUF_SIZE, hitmac_current_asn);

    logic_test(1);
    if(len > 0){
      packetbuf_set_datalen(len);
       /*send eb packet*/
      printf("send eb packet length:%d\n",len);
      NETSTACK_RADIO.send(packetbuf_dataptr(),packetbuf_datalen());

    }
    logic_test(0);

  }
  return len;
}


/*---------------------------------------------------------------------------*/
int hitmac_send_ack(){
  int len = 0;
  return len;
}
/*---------------------------------------------------------------------------*/
int hitmac_send_data(){
  int len = 0;
  return len;
}
/*---------------------------------------------------------------------------*/
int hitmac_send_cmd(){
  int len = 0;
  return len;
}

/*---------------------------------------------------------------------------*/
/*time synchronize 1:asscoiate 0:not associate*/
/*update asn operation*/
void 
update_asn()
{
   
   
   logic_test(1);
   rtimer_clock_t asn_diff;
   uint32_t mod_res;
   mod_type = HITMAC_UNKNOWN_TYPE;

   HITMAC_ASN_INC(hitmac_current_asn,1);

   HITMAC_ASN_MOD(hitmac_current_asn,HITMAC_EB_PERIOD,mod_res);

   if(mod_res == 0){
     mod_type = HITMAC_SYNC_TYPE;
   }

   PRINTF("current asn ms:%u  ls:%lu\n",hitmac_current_asn.ms1b,hitmac_current_asn.ls4b);

   PRINTF("mod_res:%lu\n",mod_res);

   PRINTF("mod_type1:%lu\n",mod_type);

   if(hitmac_is_associated == 1){
    /*for nodes passive synchronization*/
    
    if(hitmac_is_root){

      rtimer_set(&asn_rtimer,RTIMER_NOW() + HITMAC_ASN_PERIOD,0,update_asn,NULL);
    }else{
      asn_diff = 14;
      rtimer_set(&asn_rtimer,RTIMER_NOW() + HITMAC_ASN_PERIOD - asn_diff,0,update_asn,NULL);
    }
    process_post_synch(&hitmac_scheduler_process, PROCESS_EVENT_POLL, NULL);
    
   }
   logic_test(0);
   current_asn_time = RTIMER_NOW();
   
}
/*---------------------------------------------------------------------------*/
void hitmac_receive_eb()
{
  /*nodes receive eb sync packet,if not received, nodes will increase lost_sync_num*/
  int is_packet_pending = 0;
  int eb_len;
  uint8_t input_eb[HITMAC_PACKET_EB_LENGTH];
  struct ieee802154_ies eb_ies;
  frame802154_t frame;

  NETSTACK_RADIO.on();
  rtimer_clock_t t0;
  is_packet_pending = NETSTACK_RADIO.pending_packet();

  /* If we are currently receiving a packet, wait until end of reception */
  t0 = RTIMER_NOW();
  /*Wait  for two timslots for EB*/
  BUSYWAIT_UNTIL_ABS((is_packet_pending = NETSTACK_RADIO.pending_packet()), t0,HITMAC_LISTEN_WAIT_EB_MAX_LENGTH);
  
  eb_len = NETSTACK_RADIO.read(input_eb, HITMAC_PACKET_EB_LENGTH);
  if(hitmac_packet_parse_eb(input_eb,eb_len,&frame,&eb_ies)!=0){   
    /*update current nodes asn*/
    hitmac_current_asn.ms1b = eb_ies.ie_asn.ms1b;
    hitmac_current_asn.ls4b = eb_ies.ie_asn.ls4b;
    /*reset asn rtimer*/
    rtimer_set(&asn_rtimer,RTIMER_NOW() + HITMAC_REQ_EB_WAIT_TIMEOFFSET1,0,update_asn,NULL);
    PRINTF("nodes current asn ms:%u  ls:%lu\n",hitmac_current_asn.ms1b, hitmac_current_asn.ls4b);
    PRINTF("nodes passive receive asn ms:%u  ls:%lu\n",eb_ies.ie_asn.ms1b,eb_ies.ie_asn.ls4b);
    leds_toggle(LEDS_RED);
    lost_sync_num = 0;
  }
  else{
    /*disassociate from network because not receive eb packet*/  
    lost_sync_num ++;
    if(lost_sync_num>=HITMAC_LOST_SYNC_MAX_NUM){
      hitmac_is_associated = 0;
      printf("nodes disassociate from network\n");
      process_post(&hitmac_process, PROCESS_EVENT_POLL, NULL);
    }  
  }

  NETSTACK_RADIO.off();

}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(hitmac_root_eb_process,ev,data)
{
  PROCESS_BEGIN();


  while(1){
    PROCESS_YIELD();

    /* Prepare the EB packet and schedule it to be sent,50 kbps we use rtimer wait,but 5k,we will use etimer*/
    rtimer_clock_t wt;
    wt = RTIMER_NOW();
    while(RTIMER_CLOCK_LT(RTIMER_NOW(), wt + HITMAC_EB_TIMEOFFSET)) { }

    hitmac_send_packet(FRAME802154_BEACONFRAME);

  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hitmac_scheduler_process,ev,data)
{
  PROCESS_BEGIN();

  while(1){
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    PRINTF("scheduler poll\n");
    if(mod_type == HITMAC_SYNC_TYPE && hitmac_is_root == 1){
      /*root send eb sync packet*/
      process_poll(&hitmac_root_eb_process);
      //process_post
    }else if(mod_type == HITMAC_SYNC_TYPE && hitmac_is_root == 0){
      hitmac_receive_eb();
      PRINTF("prepare to receive eb sync packet\n");
    }else{

    }


  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/* Scanning protothread, called by hitmac_process:
 * Listen to an channels, and when receiving an EB,
 * attempt to associate.
 */
PT_THREAD(hitmac_request(struct pt *pt))
{
  PT_BEGIN(pt);

  static struct etimer scan_timer;
  static uint8_t rand_req = 0;
  etimer_set(&scan_timer,HITMAC_SCAN_PERIOD);

  while(!hitmac_is_associated) {
    int is_packet_pending = 0;
    int eb_len;
    uint8_t input_eb[HITMAC_PACKET_EB_LENGTH];
    struct ieee802154_ies eb_ies;
    frame802154_t frame;
    int request_len;

    /*Send request to root*/
    /* Prepare the REQUEST packet and schedule it to be sent */
    packetbuf_clear();
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_CMDFRAME);

    request_len = hitmac_packet_create_request_associate(packetbuf_dataptr(), PACKETBUF_SIZE, 0xFFFF);
    // logic_test(1);
    /* Turn radio on and wait for EB */
    NETSTACK_RADIO.on();
    
    if(request_len > 0){
      packetbuf_set_datalen(request_len);
       /*send cmd packet*/
      printf("periodically send cmd request packet length:%d\n",request_len);
      /*Send request packet*/
      NETSTACK_RADIO.send(packetbuf_dataptr(),packetbuf_datalen());
    }

    /* We are not coordinator, try to associate */
    rtimer_clock_t t0;
    is_packet_pending = NETSTACK_RADIO.pending_packet();

    /* If we are currently receiving a packet, wait until end of reception */
    t0 = RTIMER_NOW();
    /*Wait  for one timslots for EB*/

    BUSYWAIT_UNTIL_ABS((is_packet_pending = NETSTACK_RADIO.pending_packet()), t0,HITMAC_LISTEN_SYNC_WAIT_TIME);
    /*key operation*/
    NETSTACK_RADIO.off();

    // logic_test(0);
    rand_req ++;
    if(is_packet_pending) {
      eb_len = NETSTACK_RADIO.read(input_eb, HITMAC_PACKET_EB_LENGTH);
      if(hitmac_packet_parse_eb(input_eb,eb_len,&frame,&eb_ies)!=0){
        printf("nodes possive receive asn ms:%u  ls:%lu\n",eb_ies.ie_asn.ms1b,eb_ies.ie_asn.ls4b);
        hitmac_is_associated = 1;
        /*update current nodes asn*/
        hitmac_current_asn.ms1b = eb_ies.ie_asn.ms1b;
        hitmac_current_asn.ls4b = eb_ies.ie_asn.ls4b;
        /*wait a timeoffset until next slot to correct update*/
        t0 = RTIMER_NOW();
        while(RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + HITMAC_REQ_EB_WAIT_TIMEOFFSET)) { }
        /*hitmac_request pt will exit, go back on hitmac process*/
      }
    }
    
    if(!hitmac_is_associated) {
      /* Go back to scanning,nodes will promise root seceive request during up timeslot,*/
      if(rand_req%2 == 1){
        etimer_set(&scan_timer,HITMAC_SCAN_PERIOD + HITMAC_UP_SYNC_ETIMER_TIMEOFFSET);
      }else{
        etimer_set(&scan_timer,HITMAC_SCAN_PERIOD);
      }
      
      PT_WAIT_UNTIL(pt, etimer_expired(&scan_timer));
    }
     
    
  }

  PT_END(pt);
}

/*---------------------------------------------------------------------------*/
/*The main hitmac process*/
PROCESS_THREAD(hitmac_process, ev, data)
{
  PROCESS_BEGIN();

  static struct pt request_pt;

  while(1){
   
    while(!hitmac_is_associated){
      if(!hitmac_is_root){
        /*periodically send eb reuqest to root,periodically in request_pt,
        not go back to hitmac process only when hitmac_is_associated = 1*/
        PROCESS_PT_SPAWN(&request_pt,hitmac_request(&request_pt));
        
      }else
      {
        hitmac_is_associated = 1;
        process_start(&hitmac_root_eb_process,NULL);
      }
    }
    /*start scheduler process*/
    process_start(&hitmac_scheduler_process,NULL);

    /*we start asn operation*/
    update_asn();

    /*make hitmac process block,make rooms for cpu to sleep*/
    PROCESS_YIELD_UNTIL(!hitmac_is_associated);
    
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
static void
hitmac_init()
{
  hitmac_is_root = ROOTNODE;

  hitmac_is_started = 0;

  hitmac_is_associated = 0;

  HITMAC_ASN_INIT(hitmac_current_asn,0,0);

  HITMAC_SCHDELER_INIT(hitmac_current_scheduler,HITMAC_UPLOAD_LENGTH, 
    HITMAC_DOWNLOAD_LENGTH,HITMAC_SLEEP_TIME_LENGTH,HITMAC_TIME_LENGTH,HITMAC_EB_PERIOD);

#if HITMAC_AUTO_START
  NETSTACK_MAC.on();
#endif

  PRINTF("start hitmac\n");
  
}
/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{
  
}

/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
  frame802154_t frame;
  int ret;

  if((ret=frame802154_parse(packetbuf_dataptr(),packetbuf_datalen(),&frame)) == 0) {
    PRINTF("HITMAC: failed to parse frame\n");
    return ;
  }

  if(frame.fcf.frame_type == FRAME802154_CMDFRAME && hitmac_packet_is_broadcast(frame.dest_addr))
  {
    /*is FRAME802154_REQUEST_ASSOCIATE_CMDID?*/
    frame802154_t frame1;
    uint8_t cmd;

    hitmac_packet_parse_cmd(packetbuf_dataptr(),packetbuf_datalen(),&frame1,&cmd);
    printf("root receive cmd type:%2x,src addr %2x %2x\n",cmd,frame1.src_addr[0],frame1.src_addr[1]);


    if(cmd == FRAME802154_REQUEST_ASSOCIATE_CMDID){

      receive_request_time = RTIMER_NOW();

      rtimer_clock_t relative_timeoffset;
      if(receive_request_time > current_asn_time){
        relative_timeoffset = receive_request_time - current_asn_time;
      }else{
        relative_timeoffset = 0xFFFFFFFF - current_asn_time +receive_request_time;
      }
      if(relative_timeoffset<=HITMAC_UP_SYNC_RTIMER_TIMEOFFSET)
      {        
        /*current slot send */
        rtimer_clock_t wt,left_timeoffset = 0;

        wt = RTIMER_NOW();
        
        if((current_asn_time+HITMAC_EB_TIMEOFFSET)>receive_request_time){
          left_timeoffset = (current_asn_time+HITMAC_EB_TIMEOFFSET)-receive_request_time;
        }else{
          left_timeoffset = (0xFFFFFFFF-(current_asn_time+HITMAC_EB_TIMEOFFSET)) + receive_request_time;
        }
        
        while(RTIMER_CLOCK_LT(RTIMER_NOW(), wt + left_timeoffset)) { }

        hitmac_send_packet(FRAME802154_BEACONFRAME);

        leds_toggle(LEDS_RED);

        printf("root respond\n");
      }

    }
    
  }
  
  // int request_len;
  // /* Prepare the REQUEST packet and schedule it to be sent */
  // packetbuf_clear();
  // packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_CMDFRAME);

  // request_len = hitmac_packet_create_request_associate(packetbuf_dataptr(), PACKETBUF_SIZE, 0xFFFF);

  // if(request_len > 0){
  //   packetbuf_set_datalen(request_len);
  //    /*send cmd packet*/
  //   printf("send cmd packet length:%d\n",request_len);
  // }


  // frame802154_t frame1;
  // uint8_t cmd;
  // hitmac_packet_parse_cmd(packetbuf_dataptr(),packetbuf_datalen(),&frame1,&cmd);
  // printf("receive cmd packet length:%d\n",packetbuf_datalen());
  // printf("cmd type:%2x\n",cmd);

  /**********************************************/


  // /* PRINTF("TSCH: EB received\n"); */
  // frame802154_t frame;
  // /* Verify incoming EB (does its ASN match our Rx time?),
  //  * and update our join priority. */
  // struct ieee802154_ies eb_ies;

  // hitmac_packet_parse_eb(packetbuf_dataptr(),packetbuf_datalen(),&frame,&eb_ies);
  // printf("receive asn ms:%u  ls:%lu\n",eb_ies.ie_asn.ms1b,eb_ies.ie_asn.ls4b);

  
}
/*---------------------------------------------------------------------------*/
static int
turn_on(void)
{
  if(hitmac_is_started == 0 ) {
 
    process_start(&hitmac_process, NULL);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
turn_off(int keep_radio_on)
{
  if(keep_radio_on) {
    NETSTACK_RADIO.on();
  } else {
    NETSTACK_RADIO.off();
  }
  return 1;
}

/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct mac_driver hitmac_driver = {
  "HITMAC",
  hitmac_init,
  send_packet,
  packet_input,
  turn_on,
  turn_off,
  channel_check_interval,
};
/*---------------------------------------------------------------------------*/
