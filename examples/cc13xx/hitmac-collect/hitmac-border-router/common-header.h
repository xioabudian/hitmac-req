#ifndef _COMMON_HEADER_H_
#define _COMMON_HEADER_H_
#include "contiki.h"
/*---------------------------------------------------------------------------*/
// # 系统监测类指令  参数上报 18+3+2+24+1 =48

#define CMD_SYSTEM_MONITOR      0x00  

//#define INDEX_SCHEDULE  		    0 //18	    //   调度信息   18byte schedule 
#define INDEX_TIME                  0//3       //   时间同步   3 byte time
#define INDEX_PARENT  				3//2	    //   网络拓扑            2 byte    parent id
#define INDEX_ENERGYCOST  		    5//24	    //   能耗              4*6 byte   record 200+years
#define INDEX_ADCVOLTAGE  		    29//2	    //   1.采样电压            2 byte    true or false 

#define INDEX_BEACON_INTERVAL		31//4       //   2. 时间差:to sync tick
#define INDEX_RTMETRIC				35//1       //   3.RSSI:change to rssi
#define INDEX_TIME_DIFF				37//1		//   5. change to channel number
#define INDEX_RESTART_COUNT         38//1       //   4.reboot number
#define INDEX_NUM_NEIGHBORS			40//2       //   6. 节点环境噪声（dbm）

#define INDEX_PARENTRSSI            39 //2
#define INDEX_IRQ                   41 //6
#define INDEX_NETSYN_SOURCEID                                       47//2
#define INDEX_NETSYN_RECVTIME                                       49//3
#define INDEX_NETSYN_RECVSEQNUM                                       52//1
#define INDEX_NETSYN_RECVLEVEL                                      53//1

#define INDEX_AUTOCAL_INTERVAL       54    //4
#define INDEX_CAL_OFFSET             58    //2
#define INDEX_END                    60

#define SYSTEM_MONITOR_MSG_LENGTH (INDEX_END)

void get_system_monitor_msg(uint8_t array[],int length);

#endif