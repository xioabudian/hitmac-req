#ifndef __HITMAC_CONF_H__
#define __HITMAC_CONF_H__
#include "contiki-conf.h"


#define ACK_LEN 3
#define HITMAC_PACKET_EB_LENGTH 20
#define HITMAC_PACKET_MAX_LEN MIN(127,PACKETBUF_SIZE)
/******** Configuration *******/

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


#if HITMAC_CONF_DEFAULT_SCHEDULER_LENGTH == 18000 //15 minutes
/*each slot time length ms*/
#define HITMAC_CONF_DEFAULT_TIMESLOT_LENGTH (RTIMER_SECOND/20) //50ms
/*define asn increase time*/
#define HITMAC_ASN_PERIOD HITMAC_CONF_DEFAULT_TIMESLOT_LENGTH

/*define hitmac default scheduler*/
#define HITMAC_RETRANSMITS_MAX_NUM 3
#define HITMAC_NODES_NUM 256
#define HITMAC_UPLOAD_LENGTH (HITMAC_RETRANSMITS_MAX_NUM*HITMAC_NODES_NUM) //38.4s (256*3)
#define HITMAC_DOWNLOAD_LENGTH (6000) //5minutes (6000)
#define HITMAC_SLEEP_TIME_LENGTH  (HITMAC_CONF_DEFAULT_SCHEDULER_LENGTH-HITMAC_UPLOAD_LENGTH-HITMAC_DOWNLOAD_LENGTH)
#define HITMAC_TIME_LENGTH HITMAC_CONF_DEFAULT_SCHEDULER_LENGTH


#define HITMAC_EB_PERIOD  600//root send eb packet period is 30 second
/*record current eb timeoffset when meet responding request or sending sync eb*/
#define HITMAC_EB_TIMEOFFSET (RTIMER_SECOND/36) //28 35 ms
#define HITMAC_LISTEN_SYNC_WAIT_TIME (HITMAC_ASN_PERIOD)
#define HITMAC_LISTEN_ACK_WAIT_TIME (RTIMER_SECOND/100)
#define HITMAC_LISTEN_WAIT_EB_MAX_LENGTH (RTIMER_SECOND/15)
#define HITMAC_LOST_SYNC_MAX_NUM 1

#define HITMAC_REQ_EB_WAIT_TIMEOFFSET (RTIMER_SECOND/78)

#define HITMAC_REQ_EB_WAIT_TIMEOFFSET1 (RTIMER_SECOND/60)

#define HITMAC_SCAN_PERIOD  (CLOCK_SECOND*20)//nodes request associate period  is 30s
//according to HITMAC_ASN_PERIOD set HITMAC_UP_SYNC_TIMEOFFSET.
#define HITMAC_UP_SYNC_ETIMER_TIMEOFFSET (CLOCK_SECOND/40)
#define HITMAC_UP_SYNC_RTIMER_TIMEOFFSET (HITMAC_ASN_PERIOD/2)

#define HITMAC_RSSI_THRESHOLD 0xA6 /* 0xBA<==>-70  0xB0<==>-80// 0xA6<==>-90 */

#define HITMAC_SYNC_TYPE 0
#define HITMAC_UPLOAD_TYPE 1
#define HITMAC_DOWNLOAD_TYPE 2
#define HITMAC_SLEEP_TYPE 3
#define HITMAC_UNKNOWN_TYPE 0xFF

#else
#error "NOT SUPPORT THIS SCHEDULER LENGTH"
#endif



#endif