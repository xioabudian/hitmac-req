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

#define DEBUG_JOIN_NET 0

#define DEBUG_SYNC 0

#define DEBUG_UPLOAD 0

#define DEBUG_DOWNLOAD 0

#define DEBUG_OTHER 0

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
/*down number*/
static uint32_t down_schedule_num = 0;

static uint8_t mod_type;

static uint8_t hitmac_is_broadcast = 2;
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
/*on a timeslot , beacon immediately send,data later send*/
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
#if DEBUG_SYNC
      printf("hitmac send eb packet length:%d\n",len);
      PRINTF("current asn: %lu\n", hitmac_current_asn.ls4b);
#endif
    }
    

  }else if(type == FRAME802154_DATAFRAME){
    uint8_t buf[HITMAC_PACKET_MAX_LEN];
    /*buf = head + data*/
    const linkaddr_t *addr;
    if(hitmac_is_root){
      addr = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
      len = hitmac_packet_create_dataframe(buf,HITMAC_PACKET_MAX_LEN,addr->u16);
      PRINTF("send current root addr %2x%2x\n", addr->u8[0],addr->u8[1]);
    }else{
      len = hitmac_packet_create_dataframe(buf,HITMAC_PACKET_MAX_LEN,current_root_addr.u16);
      addr = &current_root_addr;
      PRINTF("send current nodes addr %2x%2x\n", addr->u8[0],addr->u8[1]);
    }

    if(len > 0){
      packetbuf_clear();
      packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
      packetbuf_copyfrom(buf,len);

      
    }

  }
  return len;
}

/*---------------------------------------------------------------------------*/
int hitmac_nodes_send_data(){
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
      PRINTF("upload current bussiness %lu\n", hitmac_current_bussiness);
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
        return len;
      }
    }
    if(len < 0){
      p->transmissions++;
      if(p->transmissions > HITMAC_RETRANSMITS_MAX_NUM){//HITMAC_RETRANSMITS_MAX_NUM
        hitmac_queue_remove_packet();
#if DEBUG_UPLOAD
        printf("nodes send lost packet\n");
#endif 
      }
    }
    /*key operation*/
    NETSTACK_RADIO.off();
    
  }
  return len;
}
/*---------------------------------------------------------------------------*/
/*Nodes parse data and send ack*/
void hitmac_node_parse_data(){
  int hdr_len = -1;
  int duplicate = 0;

  hdr_len = NETSTACK_FRAMER.parse();
         
  if(hdr_len < 0){
    printf("HITMAC: download failed to parse frame\n");
    return ; 
  }

  /*root receive data frame*/
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)==FRAME802154_DATAFRAME){
    if(linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),&linkaddr_node_addr)==1){
      /*unicast addr*/
      uint8_t ack_len[ACK_LEN];
      ack_len[ACK_LEN-1] = packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO) +1;
      NETSTACK_RADIO.send(ack_len,ACK_LEN);
    }
    /*broadcast addr,do nothing*/
    uint8_t *original_dataptr = packetbuf_dataptr();

    /* Seqno of 0xffff means no seqno */
    if(packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO) != 0xffff) {
      /* Check for duplicates */
      duplicate = mac_sequence_is_duplicate();
      if(duplicate) {
        /* Drop the packet. */
#if DEBUG_OTHER
        printf("HITMAC:! drop dup seqno %u\n",packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
#endif
        
      } else {
        mac_sequence_register_seqno();
      }
    }

    if(!duplicate) {
#if DEBUG_DOWNLOAD
      PRINTF("HITMAC! packet input\n");
#endif
      input_buf.len = packetbuf_datalen()-hdr_len;
      memcpy(input_buf.buf,&original_dataptr[hdr_len], input_buf.len);
      memcpy(&input_buf.src_addr, packetbuf_addr(PACKETBUF_ADDR_SENDER), LINKADDR_SIZE);
      memcpy(&input_buf.dest_addr, packetbuf_addr(PACKETBUF_ADDR_RECEIVER), LINKADDR_SIZE);    
      if(conn_process!=NULL){
        process_post(conn_process, PACKET_INPUT, NULL);   
      }
    }

    return;
  }
}

/*---------------------------------------------------------------------------*/
int hitmac_root_send_data(){
  int len = -1;
  /*get packet from queue*/
  struct hitmac_packet *p = hitmac_queue_get_packet();
  uint8_t ackbuf[ACK_LEN] = {0,0,0};
  int ack_len;
  int is_packet_pending = 0;

  if(p != NULL){
    if(p->len > 0){
      /*send data*/
      logic_test(1);
      NETSTACK_RADIO.send(p->data,p->len);
      logic_test(0);
      hitmac_is_broadcast--;
      
    }
    if(p->data[5]==0xFF && p->data[6]==0xFF){
      /*broadcast addr*/
      if(hitmac_is_broadcast == 0){
        hitmac_queue_remove_packet();
      }
      
#if DEBUG_DOWNLOAD
      printf("send broadcast\n");
#endif
      return p->len;
    }


#if DEBUG_DOWNLOAD
    printf("send unicast\n");
#endif
    logic_test(1);
   /* We are not coordinator, try to wait ack */
    rtimer_clock_t t0;
    t0 = RTIMER_NOW();
    /*Wait  for one timslot for ACK*/
    BUSYWAIT_UNTIL_ABS((is_packet_pending = NETSTACK_RADIO.pending_packet()), t0,HITMAC_NODE_WAIT_DATA_TIME);
    
    if(is_packet_pending){
      ack_len = NETSTACK_RADIO.read(ackbuf,ACK_LEN);
      uint8_t seq_no = (uint8_t)p->data[2]+1;
      
      if(ack_len==ACK_LEN &&  seq_no== ackbuf[ACK_LEN-1]){
        len = p->len;
        hitmac_queue_remove_packet();
        logic_test(0);
        return len;
      }
    }
    if(len < 0){
      p->transmissions++;
      if(p->transmissions > HITMAC_RETRANSMITS_MAX_NUM){//HITMAC_RETRANSMITS_MAX_NUM
        hitmac_queue_remove_packet();
#if DEBUG_DOWNLOAD
        printf("root send lost packet\n");
#endif
        
      }
    }
    logic_test(0);
    
  }
  return len;
}
/*---------------------------------------------------------------------------*/
/*return  hitmac_current_bussiness*/
void hitmac_set_asn(struct tsch_asn_t asn){
  uint32_t res =0;
  uint32_t a,b,c;
  hitmac_current_asn.ms1b = asn.ms1b;
  hitmac_current_asn.ls4b = asn.ls4b;
  
//patch 6 years
  down_schedule_num = 3;
  a = asn.ls4b;
  b = HITMAC_EB_PERIOD;
  c = a-(a+b-1)/b;
  res = c % HITMAC_TIME_LENGTH;

  hitmac_current_bussiness = res;
#if DEBUG_OTHER
  printf("bus %lu\n", res);
#endif
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
      business_res = hitmac_current_bussiness;

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
    logic_test(0);

    process_post_synch(&hitmac_scheduler_process, PROCESS_EVENT_POLL, NULL);
    
   }
   current_asn_time = RTIMER_NOW();
   
}
/*---------------------------------------------------------------------------*/
uint8_t 
get_mod_type(){
  uint8_t res;
  res = mod_type;

  if(!hitmac_is_associated){
    
    return HITMAC_UNKNOWN_TYPE;
  }
  return res;
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
  /*Wait  for one timslot for EB*/
  BUSYWAIT_UNTIL_ABS((is_packet_pending = NETSTACK_RADIO.pending_packet()), t0,HITMAC_LISTEN_WAIT_EB_MAX_LENGTH);
  
  eb_len = NETSTACK_RADIO.read(input_eb, HITMAC_PACKET_EB_LENGTH);
  logic_test(0);
  if(hitmac_packet_parse_eb(input_eb,eb_len,&frame,&eb_ies)!=0){   
    /*update current nodes asn*/
    /*according surrent asn to calculate current_bussiness*/
    hitmac_set_asn(eb_ies.ie_asn);

    /*reset asn rtimer*/
    rtimer_set(&asn_rtimer,RTIMER_NOW() + HITMAC_REQ_EB_WAIT_TIMEOFFSET1,0,update_asn,NULL);
#if DEBUG_SYNC
    PRINTF("nodes current asn ms:%u  ls:%lu\n",hitmac_current_asn.ms1b, hitmac_current_asn.ls4b);
    PRINTF("nodes passive receive asn ms:%u  ls:%lu\n",eb_ies.ie_asn.ms1b,eb_ies.ie_asn.ls4b);
    leds_toggle(LEDS_RED);
#endif
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
    down_schedule_num = 3;
#if DEBUG_SYNC
    printf("send eb packet\n");
#endif
#if DEBUG_OTHER
    printf("root bus %lu\n", hitmac_current_bussiness);
#endif
    
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hitmac_scheduler_process,ev,data)
{
  PROCESS_BEGIN();
  /*hitmac download phrase schedule*/
  uint8_t down_schedule;

  static uint8_t next_slot_packet_seen =0;
  int is_packet_pending = 0;
  int len;
  static rtimer_clock_t start;
  static uint8_t strobes;
  static char down_buf[HITMAC_DOWNLOAD_BUF_TIME]="wake nodes packets";
  memset(down_buf,0,HITMAC_DOWNLOAD_BUF_TIME);

  while(1){
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    switch(mod_type){

      case HITMAC_SYNC_TYPE: 
        if(hitmac_is_root == 1){
          /*root send eb sync packet*/
          process_poll(&hitmac_root_eb_process);
          //process_post
        }else{
          hitmac_receive_eb();
        }

      break;

      case HITMAC_UPLOAD_TYPE: 
        if(!hitmac_is_root){
          if(hitmac_current_bussiness % HITMAC_NODES_NUM == hitmac_slot){
            hitmac_nodes_send_data();
            
          }
        }
      break;

      case HITMAC_DOWNLOAD_TYPE: 
        
        down_schedule= down_schedule_num % HITMAC_DOWNLOAD_FREQUENCY;

        if(hitmac_is_root ==1){
          if(down_schedule==0){
            /*root send a series of packets until waited ack*/
            // rtimer_clock_t t0;
            struct hitmac_packet *p = hitmac_queue_get_packet();
            if(p!=NULL && p->len>0){
              start = RTIMER_NOW();
              for(strobes=0;RTIMER_CLOCK_LT(RTIMER_NOW(), start + HITMAC_ROOT_DOWNLOAD_WAKE_TIME); 
                strobes++){
                logic_test(1);
                NETSTACK_RADIO.send(down_buf,sizeof(down_buf));
                logic_test(0);
                // t0 = RTIMER_NOW();packet_interval = 8.83ms - 6.4ms =2.43ms
                // while(RTIMER_CLOCK_LT(RTIMER_NOW(),(t0+HITMAC_PACKET_INTERVAL))){}
                /*wait next cycle*/
              }
            }

          }else if(down_schedule==1){
            /*start send time*/
            start = RTIMER_NOW();
            while(RTIMER_CLOCK_LT(RTIMER_NOW(),(start+HITMAC_ROOT_DOWNLOAD_START_TIME))){}
            /*root send a packet until waited ack*/
            hitmac_root_send_data();
          }
        }else{
          if(down_schedule==0){
            /*nodes do 2 times cca*/
            start = RTIMER_NOW();
            while(RTIMER_CLOCK_LT(RTIMER_NOW(),(start+HITMAC_CCA_START_TIME))){}
            NETSTACK_RADIO.on();
            logic_test(1);
            if(NETSTACK_RADIO.channel_clear() == 0){
              next_slot_packet_seen ++;
            }
            logic_test(0);
            NETSTACK_RADIO.off();
            start = RTIMER_NOW();
            while(RTIMER_CLOCK_LT(RTIMER_NOW(),(start+HITMAC_CCA_SLEEP_TIME))){}
            NETSTACK_RADIO.on();
            logic_test(1);
            if(NETSTACK_RADIO.channel_clear() == 0){
              next_slot_packet_seen ++;
            }
            logic_test(0);
            NETSTACK_RADIO.off();
            
          }else if(down_schedule==1){
           
            if(next_slot_packet_seen > 0){
              /*nodes wait for data and return a ack*/
              logic_test(1);
              NETSTACK_RADIO.on();
              start = RTIMER_NOW();
              /*wait for data frame*/
              BUSYWAIT_UNTIL_ABS((is_packet_pending=NETSTACK_RADIO.pending_packet()),start,HITMAC_NODE_WAIT_DATA_TIME);
              /*parse data*/
              if(is_packet_pending){
                /*receive data */   
                packetbuf_clear();
                len = NETSTACK_RADIO.read(packetbuf_dataptr(), PACKETBUF_SIZE);
                if( len>0 ){
                  packetbuf_set_datalen(len);
                }
                /*parse data and send ack*/
                /*input packet into nodes app*/
                hitmac_node_parse_data();
              }
              NETSTACK_RADIO.off();
              logic_test(0);
#if DEBUG_OTHER
              PRINTF("packet_seen:%d\n",next_slot_packet_seen);
#endif
              

            }else{
              NETSTACK_RADIO.off();
            }
          }else{
            next_slot_packet_seen =0;
            
          }
        }

        down_schedule_num ++;

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
#if DEBUG_JOIN_NET
      printf("periodically send cmd request packet length:%d\n",request_len);
#endif
      packetbuf_set_datalen(request_len);
      /*send cmd request packet*/
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
#if DEBUG_JOIN_NET
        printf("nodes possive receive asn ms:%u  ls:%lu\n",eb_ies.ie_asn.ms1b,eb_ies.ie_asn.ls4b);
#endif
        leds_toggle(LEDS_RED);
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
    if(addr->u16 ==0xFFFF){
      hitmac_is_broadcast = HITMAC_DOWNLOAD_BROADCAST_MAX_NUM;
    }
    PRINTF("send current root addr %2x%2x\n", addr->u8[0],addr->u8[1]);
  }

  if(!hitmac_is_associated) {
    PRINTF("HITMAC:! not associated, drop outgoing packet\n");
    ret = MAC_TX_ERR;
    mac_call_sent_callback(sent, ptr, ret, 1);
    return;
  }

  if((buf_len = hitmac_send_packet(FRAME802154_DATAFRAME)) < 0){
    printf("HITMAC:! can't send packet due to framer error\n");
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

  addr =NULL;

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
#if DEBUG_OTHER
    printf("HITMAC: failed to parse frame\n");
#endif
    return ; 
  }

  /*root receive cmd request*/
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)== FRAME802154_CMDFRAME && hitmac_is_root==1
    && linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &linkaddr_null) == 1)//broadcast
  {
    /*is FRAME802154_REQUEST_ASSOCIATE_CMDID?*/
    frame802154_t frame1;
    uint8_t cmd;

    hitmac_packet_parse_cmd(packetbuf_dataptr(),packetbuf_datalen(),&frame1,&cmd);
#if DEBUG_JOIN_NET
    printf("root receive cmd type:%2x,src addr %2x %2x\n",cmd,frame1.src_addr[0],frame1.src_addr[1]);
    PRINTF("cmd hdr len:%d\n", hdr_len);
    PRINTF("rssi: %d\n", (int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
#endif
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
#if DEBUG_JOIN_NET
        printf("root respond\n");
#endif
      }

    }

    return;
  }
  /*root receive data frame*/
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)==FRAME802154_DATAFRAME && hitmac_is_root==1
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
#if DEBUG_OTHER
        printf("HITMAC:! drop dup seqno %u\n",packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
#endif
      } else {
        mac_sequence_register_seqno();
      }
    }

    if(!duplicate) {
#if DEBUG_UPLOAD
      PRINTF("HITMAC! packet input\n");
#endif
      input_buf.len = packetbuf_datalen()-hdr_len;
      memcpy(input_buf.buf,&original_dataptr[hdr_len], input_buf.len);
      memcpy(&input_buf.src_addr, packetbuf_addr(PACKETBUF_ADDR_SENDER), LINKADDR_SIZE);
      memcpy(&input_buf.dest_addr, packetbuf_addr(PACKETBUF_ADDR_RECEIVER), LINKADDR_SIZE);    
      if(conn_process!=NULL){
        process_post(conn_process, PACKET_INPUT, NULL);   
      }
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
    hitmac_is_started = 1;
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
