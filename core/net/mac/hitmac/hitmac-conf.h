#ifndef __HITMAC_CONF_H__
#define __HITMAC_CONF_H__
#include "contiki-conf.h"


#define ACK_LEN 3
#define REQ_LEN 11 
#define HITMAC_PACKET_EB_LENGTH 20
#define HITMAC_PACKET_MAX_LEN MIN(127,PACKETBUF_SIZE)
/******** Hitmac send queue configuration *******/

/* The maximum number of outgoing packets towards each neighbor
 * Must be power of two to enable atomic ringbuf operations.
 * Note: the total number of outgoing packets in the system (for
 * all neighbors) is defined via QUEUEBUF_CONF_NUM */
#ifdef HITMAC_QUEUE_CONF_NUM
#define HITMAC_QUEUE_NUM HITMAC_QUEUE_CONF_NUM
#else
/* By default, round QUEUEBUF_CONF_NUM to next power of two
 * (in the range [4;256]) */
#if QUEUEBUF_CONF_NUM <= 4
#define HITMAC_QUEUE_NUM 4
#elif QUEUEBUF_CONF_NUM <= 8
#define HITMAC_QUEUE_NUM 8
#elif QUEUEBUF_CONF_NUM <= 16
#define HITMAC_QUEUE_NUM 16
#elif QUEUEBUF_CONF_NUM <= 32
#define HITMAC_QUEUE_NUM 32
#elif QUEUEBUF_CONF_NUM <= 64
#define HITMAC_QUEUE_NUM 64
#elif QUEUEBUF_CONF_NUM <= 128
#define HITMAC_QUEUE_NUM 128
#else
#define HITMAC_QUEUE_NUM 256
#endif
#endif


#if HITMAC_CONF_DEFAULT_SCHEDULER_LENGTH == 12000 //15 minutes
/*each slot time length ms*/
#define HITMAC_CONF_DEFAULT_TIMESLOT_LENGTH (RTIMER_SECOND/2) //500ms
/*define asn increase time*/
#define HITMAC_ASN_PERIOD HITMAC_CONF_DEFAULT_TIMESLOT_LENGTH

/*define hitmac default scheduler*/
#define HITMAC_RETRANSMITS_MAX_NUM 3
#define HITMAC_NODES_NUM 256
#define HITMAC_UPLOAD_LENGTH (6*HITMAC_NODES_NUM) //0.5s*256*6
#define HITMAC_DOWNLOAD_LENGTH (11000) //0.5s*11000
#define HITMAC_SLEEP_TIME_LENGTH  (HITMAC_CONF_DEFAULT_SCHEDULER_LENGTH-HITMAC_UPLOAD_LENGTH-HITMAC_DOWNLOAD_LENGTH)
#define HITMAC_TIME_LENGTH HITMAC_CONF_DEFAULT_SCHEDULER_LENGTH
/*define hitmac scheduler type*/
#define HITMAC_SYNC_TYPE 0
#define HITMAC_UPLOAD_TYPE 1
#define HITMAC_DOWNLOAD_TYPE 2
#define HITMAC_SLEEP_TYPE 3
#define HITMAC_UNKNOWN_TYPE 0xFF
/*define hitmac join network*/
#define HITMAC_SCAN_PERIOD  (CLOCK_SECOND*20)//nodes request associate period  is 60s
#define HITMAC_LISTEN_SYNC_WAIT_TIME (RTIMER_SECOND/13)//76ms
#define HITMAC_REQ_EB_WAIT_TIMEOFFSET (RTIMER_SECOND/2-RTIMER_SECOND/40)  //475ms
#define HITMAC_NODES_COMPLEM 16 //nodes sender complement rtimer tick,do not change this parameter
#define HITMAC_RSSI_THRESHOLD 0xA1 /* 0xBA<==>-70  0xB0<==>-80// 0xA6<==>-90 0xA1<==>-95*/
/*define hitmac eb sync*/
/*record current eb timeoffset when meet responding request or sending sync eb*/
#define HITMAC_EB_TIMEOFFSET (RTIMER_SECOND/6) //166ms
#define HITMAC_EB_PERIOD  1200//root send eb packet period is 600 second
#define HITMAC_LISTEN_WAIT_EB_MAX_LENGTH (RTIMER_SECOND/3)//333ms
#define HITMAC_LOST_SYNC_MAX_NUM 1
#define HITMAC_REQ_EB_WAIT_TIMEOFFSET1 (RTIMER_SECOND/3-RTIMER_SECOND/40)  //309ms:in a rtimer tick
/*define hitmac upload*/
#define HITMAC_LISTEN_ACK_WAIT_TIME (RTIMER_SECOND/50) //20ms
#define HITMAC_UP_SYNC_RTIMER_TIMEOFFSET (RTIMER_SECOND*2/5) //400ms
/*define hitmac download*/
#define HITMAC_DOWNLOAD_FREQUENCY 4 //

#define HITMAC_CCA_START_TIME (CLOCK_SECOND/5) //200ms
#define HITMAC_CCA_SLEEP_TIME (CLOCK_SECOND/128) //10ms
#define HITMAC_PACKET_INTERVAL (RTIMER_SECOND/1000) //1ms,real value:2.43ms
#define HITMAC_ROOT_DOWNLOAD_WAKE_TIME (RTIMER_SECOND*2/5) //400ms
#define HITMAC_ROOT_DOWNLOAD_START_TIME (RTIMER_SECOND/6) //166ms
#define HITMAC_ROOT_WAIT_ACK_TIME (RTIMER_SECOND/200) //5ms
#define HITMAC_NODE_WAIT_DATA_TIME (RTIMER_SECOND*2/5) //400ms
#define HITMAC_DOWNLOAD_BUF_TIME 50 //50ms
#define HITMAC_DOWNLOAD_BROADCAST_MAX_NUM 3

#else
#error "NOT SUPPORT THIS SCHEDULER LENGTH"
#endif



#endif