#include "contiki.h"

#include "dev/radio.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/mac/mac-sequence.h"
#include "net/mac/hitmac/hitmac.h"
#include "net/mac/hitmac/app-router.h"

#include "net/mac/hitmac/hitmac-frame.h"
#include "dev/leds.h"

#include "node-id.h"

#include <math.h>
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

#if!ROOTNODE

static linkaddr_t current_root_addr;
#endif

static int hitmac_is_associated;

static struct rtimer asn_rtimer;

static struct hitmac_asn_t hitmac_current_asn;

static struct hitmac_scheduler_t hitmac_current_scheduler;
/*bussiness phrase*/
static uint32_t hitmac_current_bussiness;

static uint8_t mod_type;
/*record current asn timeslot start point */
static rtimer_clock_t current_asn_time;
/*root receive request timeoffset*/
rtimer_clock_t receive_request_time;
/*static nodeid to slot*/
static uint8_t hitmac_slot;
/*root choose eb sending when receive request cmd*/
static int8_t rssi_threshold = HITMAC_RSSI_THRESHOLD;

uint16_t hitmac_nodeid[5]={0x0e78,0x0e53,0xf0e5};
uint8_t nodeid_slot[5] ={79,54,0xe6};

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
PROCESS(hitmac_test_process,"test process");

/*---------------------------------------------------------------------------*/
/*on a timeslot , send packet timeoffset*/
int 
hitmac_send_packet(uint8_t type){

  int len = -1;
  if(type == FRAME802154_BEACONFRAME){
    packetbuf_clear();
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_BEACONFRAME);

    len = hitmac_packet_create_eb(packetbuf_dataptr(), PACKETBUF_SIZE, hitmac_current_asn);

    if(len > 0){
      packetbuf_set_datalen(len);
       /*send eb packet*/
      logic_test(1);
      NETSTACK_RADIO.send(packetbuf_dataptr(),packetbuf_datalen());
      logic_test(0);
      printf("send eb packet length:%d\n",len);
      PRINTF("current asn: %lu\n", hitmac_current_asn.ls4b);

    }
    

  }else if(type == FRAME802154_DATAFRAME){
    uint8_t buf[HITMAC_PACKET_MAX_LEN];
    /*buf = head + data*/
    len = hitmac_packet_create_dataframe(buf,HITMAC_PACKET_MAX_LEN,current_root_addr.u16);
    
    if(len > 0){
      packetbuf_clear();
      packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
      packetbuf_copyfrom(buf,len);
    }

    printf("send data packet length:%u\n", len);
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
  int len = -1;
  /*get packet from queue*/
  struct hitmac_packet *p = hitmac_queue_get_packet();
  uint8_t ackbuf[ACK_LEN] = {0,0,0};
  int ack_len;
  int is_packet_pending = 0;

  if(p != NULL){
    NETSTACK_RADIO.on();
    if(p->len > 0){
      /*send data*/
      NETSTACK_RADIO.send(p->data,p->len);
      printf("current bussiness %lu\n", hitmac_current_bussiness);
    }
   /* We are not coordinator, try to wait ack */
    rtimer_clock_t t0;
    t0 = RTIMER_NOW();
    /*Wait  for one timslots for ACK*/
    BUSYWAIT_UNTIL_ABS((is_packet_pending = NETSTACK_RADIO.pending_packet()), t0,HITMAC_LISTEN_ACK_WAIT_TIME);
    
    if(is_packet_pending){
      ack_len = NETSTACK_RADIO.read(ackbuf,ACK_LEN);
      uint8_t seq_no = (uint8_t)p->data[2]+1;
      
      if(ack_len==ACK_LEN &&  seq_no== ackbuf[ACK_LEN-1]){
        len = p->len;
        hitmac_queue_remove_packet();
        NETSTACK_RADIO.off();
        printf("receive ack\n");
        return len;
      }
    }
    if(len < 0){
      p->transmissions++;
      if(p->transmissions > HITMAC_RETRANSMITS_MAX_NUM){//HITMAC_RETRANSMITS_MAX_NUM
        hitmac_queue_remove_packet();
        printf("nodes send lost packet\n");
      }
    }
    /*key operation*/
    NETSTACK_RADIO.off();
    
  }
  return len;
}

/*---------------------------------------------------------------------------*/
int hitmac_send_cmd(){
  int len = 0;
  return len;
}

/*---------------------------------------------------------------------------*/
/*return  hitmac_current_bussiness*/
void hitmac_set_asn(struct tsch_asn_t asn){
  uint32_t res =0;
  hitmac_current_asn.ms1b = asn.ms1b;
  hitmac_current_asn.ls4b = asn.ls4b;
  uint32_t mod_asn = get_asn_mod_val(hitmac_current_asn,HITMAC_TIME_LENGTH);
  
  if(mod_asn % HITMAC_EB_PERIOD == 0){
    res = mod_asn - mod_asn/HITMAC_EB_PERIOD;
  }else{
    res = mod_asn - mod_asn/HITMAC_EB_PERIOD - 1;
  }

  hitmac_current_bussiness = res;
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
   uint32_t business_res;

   mod_type = HITMAC_UNKNOWN_TYPE;

   mod_res = get_asn_mod_val(hitmac_current_asn,HITMAC_EB_PERIOD);

   if(mod_res == 0){
      /*synchronization phrase*/
      mod_type = HITMAC_SYNC_TYPE;
   }else{
      
      if(hitmac_current_bussiness >= HITMAC_TIME_LENGTH){
        hitmac_current_bussiness = 0;
      }
      /*business phrase*/
      business_res = hitmac_current_bussiness % HITMAC_TIME_LENGTH;

      if(business_res >= 0 && business_res < HITMAC_UPLOAD_LENGTH){
        mod_type = HITMAC_UPLOAD_TYPE;
      }else if(business_res >= HITMAC_UPLOAD_LENGTH && business_res<(HITMAC_UPLOAD_LENGTH + HITMAC_DOWNLOAD_LENGTH) ){
        mod_type = HITMAC_DOWNLOAD_TYPE;
      }
      hitmac_current_bussiness ++;
      
   }
   
   HITMAC_ASN_INC(hitmac_current_asn,1);

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
  logic_test(1);
  NETSTACK_RADIO.on();
  rtimer_clock_t t0;
  is_packet_pending = NETSTACK_RADIO.pending_packet();

  /* If we are currently receiving a packet, wait until end of reception */
  t0 = RTIMER_NOW();
  /*Wait  for two timslots for EB*/
  BUSYWAIT_UNTIL_ABS((is_packet_pending = NETSTACK_RADIO.pending_packet()), t0,HITMAC_LISTEN_WAIT_EB_MAX_LENGTH);
  
  eb_len = NETSTACK_RADIO.read(input_eb, HITMAC_PACKET_EB_LENGTH);
  logic_test(0);
  if(hitmac_packet_parse_eb(input_eb,eb_len,&frame,&eb_ies)!=0){   
    /*update current nodes asn*/
    /*according surrent asn to calculate current_bussiness*/
    hitmac_set_asn(eb_ies.ie_asn);

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
#if!ROOTNODE
      current_root_addr.u8[0] = 0xFF;
      current_root_addr.u8[1] = 0xFF;
#endif
      hitmac_queue_reset();
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

    switch(mod_type){

      case HITMAC_SYNC_TYPE: 
        if(hitmac_is_root == 1){
          /*root send eb sync packet*/
          process_poll(&hitmac_root_eb_process);
          //process_post
        }else{
          hitmac_receive_eb();
          PRINTF("prepare to receive eb sync packet\n");
        }

      break;

      case HITMAC_UPLOAD_TYPE: 
        if(!hitmac_is_root){
          if(hitmac_current_bussiness % HITMAC_NODES_NUM == hitmac_slot){
            hitmac_send_data();
            
          }
        }
      break;

      case HITMAC_DOWNLOAD_TYPE: 
        if(hitmac_is_root ==1){

        }else{

        }

      break;

      default:PRINTF("sleep phrase\n");
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
    
    if(is_packet_pending) {
      eb_len = NETSTACK_RADIO.read(input_eb, HITMAC_PACKET_EB_LENGTH);
      if(hitmac_packet_parse_eb(input_eb,eb_len,&frame,&eb_ies)!=0){
        printf("nodes possive receive asn ms:%u  ls:%lu\n",eb_ies.ie_asn.ms1b,eb_ies.ie_asn.ls4b);
        hitmac_is_associated = 1;
#if!ROOTNODE
        current_root_addr.u8[0] = frame.src_addr[0];
        current_root_addr.u8[1] = frame.src_addr[1];
#endif       
        /*update current nodes asn*/
        /*according surrent asn to calculate current_bussiness*/
        hitmac_set_asn(eb_ies.ie_asn);
        /*wait a timeoffset until next slot to correct update*/
        t0 = RTIMER_NOW();
        while(RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + HITMAC_REQ_EB_WAIT_TIMEOFFSET)) { }
        /*hitmac_request pt will exit, go back on hitmac process*/
      }
    }
    /*key operation*/
    NETSTACK_RADIO.off();

    rand_req ++;
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
//@test process
PROCESS_THREAD(hitmac_test_process, ev , data){
  PROCESS_BEGIN();
  static struct etimer test_et;
  etimer_set(&test_et,CLOCK_SECOND);

  while(1){

    PROCESS_YIELD();

    if(etimer_expired(&test_et) && ev == PROCESS_EVENT_TIMER){
       etimer_set(&test_et,CLOCK_SECOND * 8);
       
       printf("current asn: %lu\n", hitmac_current_asn.ls4b);
       printf("current bussiness %lu\n", hitmac_current_bussiness);
       printf("phrase: %u\n",mod_type);
    }

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

  hitmac_slot = (uint8_t)node_id & 0x00FF;

#if!ROOTNODE
  current_root_addr.u8[0] = 0xFF;
  current_root_addr.u8[1] = 0xFF;
#endif
  /*static assign time slots*/
  for(int i =0;i<3; i++){
    if(node_id == hitmac_nodeid[i]){
       hitmac_slot = nodeid_slot[i] ;
       break;
    }
  }
   
  hitmac_queue_init();

  printf("hitmac slot %x\n",hitmac_slot);

  HITMAC_ASN_INIT(hitmac_current_asn,0,0);

  hitmac_current_bussiness = 0xFFFFFFFF;

  HITMAC_SCHDELER_INIT(hitmac_current_scheduler,HITMAC_UPLOAD_LENGTH, 
    HITMAC_DOWNLOAD_LENGTH,HITMAC_SLEEP_TIME_LENGTH,HITMAC_TIME_LENGTH,HITMAC_EB_PERIOD);

#if HITMAC_AUTO_START
  NETSTACK_MAC.on();
#endif

#if 0
  process_start(&hitmac_test_process,NULL);
#endif
  printf("start hitmac\n");
  
}
/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{

  int ret = MAC_TX_DEFERRED;
  int buf_len = 0;
  
  const linkaddr_t *addr;
  if(!hitmac_is_root){
    addr = &current_root_addr;
    PRINTF("send current nodes addr %2x%2x\n", addr->u8[0],addr->u8[1]);
  }else{
    addr = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
    PRINTF("send current root addr %2x%2x\n", addr->u8[0],addr->u8[1]);
  }

  if(!hitmac_is_associated) {
    PRINTF("HITMAC:! not associated, drop outgoing packet\n");
    ret = MAC_TX_ERR;
    mac_call_sent_callback(sent, ptr, ret, 1);
    return;
  }

  if((buf_len = hitmac_send_packet(FRAME802154_DATAFRAME)) < 0){
    PRINTF("HITMAC:! can't send packet due to framer error\n");
    ret = MAC_TX_ERR;
  }else{
    int put_index;
    put_index = hitmac_queue_add_packet(sent,ptr);
    
    if(put_index == -1){
      ret = MAC_TX_ERR;
      printf("HITMAC:! can't send packet to %2x%2x\n",
          addr->u8[0],addr->u8[1]);
    }
    
  }
  
  if(ret != MAC_TX_DEFERRED) {
    mac_call_sent_callback(sent, ptr, ret, 1);
  }

}

/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
  int hdr_len = -1;
  int duplicate = 0;
  hdr_len = NETSTACK_FRAMER.parse();

  if(hdr_len < 0){
    printf("HITMAC: failed to parse frame\n");
    return ; 
  }


  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)== FRAME802154_CMDFRAME 
    && linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &linkaddr_null) == 1)//broadcast
  {
    /*is FRAME802154_REQUEST_ASSOCIATE_CMDID?*/
    frame802154_t frame1;
    uint8_t cmd;

    hitmac_packet_parse_cmd(packetbuf_dataptr(),packetbuf_datalen(),&frame1,&cmd);
    
    printf("root receive cmd type:%2x,src addr %2x %2x\n",cmd,frame1.src_addr[0],frame1.src_addr[1]);
    PRINTF("cmd hdr len:%d\n", hdr_len);
    PRINTF("rssi: %d\n", (int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
    /*request cmd && rssi threshold*/
    if(cmd == FRAME802154_REQUEST_ASSOCIATE_CMDID &&(int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI) >= rssi_threshold){

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

    return;
  }
  
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)==FRAME802154_DATAFRAME 
    && linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),&linkaddr_node_addr)==1){
    uint8_t ack_len[ACK_LEN];
    ack_len[ACK_LEN-1] = packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO) +1;
   
    uint8_t *original_dataptr = packetbuf_dataptr();

    NETSTACK_RADIO.send(ack_len,ACK_LEN);

    PRINTF("frame hdr len:%d\n", hdr_len);
    PRINTF("frame total len:%d\n", original_datalen);

    /* Seqno of 0xffff means no seqno */
    if(packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO) != 0xffff) {
      /* Check for duplicates */
      duplicate = mac_sequence_is_duplicate();
      if(duplicate) {
        /* Drop the packet. */
        printf("HITMAC:! drop dup seqno %u\n",packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
        
      } else {
        mac_sequence_register_seqno();
      }
    }

    if(!duplicate) {
      PRINTF("HITMAC! packet input\n");
      input_buf.len = packetbuf_datalen()-hdr_len;
      memcpy(input_buf.buf,&original_dataptr[hdr_len], input_buf.len);
      memcpy(&input_buf.src_addr, packetbuf_addr(PACKETBUF_ADDR_SENDER), LINKADDR_SIZE);
      memcpy(&input_buf.dest_addr, packetbuf_addr(PACKETBUF_ADDR_RECEIVER), LINKADDR_SIZE);    
      
      process_post(conn_process, PACKET_INPUT, NULL);   
    }

    return;
  }
  
  
  
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
