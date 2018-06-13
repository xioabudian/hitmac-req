#include "common-header.h"
#include "bat-voltage.h"
#include "node-id.h"
#include "net/mac/hitmac/hitmac.h"
#include "net/netstack.h"

#define DEBUG 0
#if DEBUG 
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
static void set_energy(int index,uint8_t array[],uint64_t val)
{
  int i ;
  for (i=5; i >=0; i--)
  {
    /* code */
    array[index+i]=0;
  }

  for (i=5; i >=0; i--)
  {
    /* code */
    array[index+i]=val&0xff;
    val=val>>8;
  }
}
/*system monitoring instruction*/
/*---------------------------------------------------------------------------*/
void get_system_monitor_msg(uint8_t array[],int length)
{

  unsigned long cpu,lpm,transmit,listen,irq;
  uint16_t temp_votlage=0;
  /*record hitmac sync diff tick numbers*/
  uint32_t temp_sync_diff;

  if (length<SYSTEM_MONITOR_MSG_LENGTH)
  {
    PRINTF("node_function.c array error msg \n");
    return;
  }

  // array[INDEX_TIME]  =   'B';
  // array[INDEX_TIME+1]=   'B';
  // array[INDEX_TIME+2]=   'B';
  

  // array[INDEX_NETSYN_SOURCEID]  =0; 
  // array[INDEX_NETSYN_SOURCEID+1]  =0; 

  // array[INDEX_NETSYN_RECVTIME]=   0;
  // array[INDEX_NETSYN_RECVTIME+1]=   0;
  // array[INDEX_NETSYN_RECVTIME+2]=   0;
  // array[INDEX_NETSYN_RECVSEQNUM]  =0;
  // array[INDEX_NETSYN_RECVLEVEL]  =0;

  energest_flush();
  cpu      = energest_type_time(ENERGEST_TYPE_CPU)      ;
  lpm      = energest_type_time(ENERGEST_TYPE_LPM)      ;
  transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT) ;
  listen   = energest_type_time(ENERGEST_TYPE_LISTEN)   ;
  irq     = energest_type_time(ENERGEST_TYPE_IRQ);

  PRINTF("cpu  %lu  , lpm  %lu , transmit %lu , listen %lu  \n", cpu,lpm,transmit,listen );

  energest_type_set(ENERGEST_TYPE_CPU,0);
  energest_type_set(ENERGEST_TYPE_LPM,0);
  energest_type_set(ENERGEST_TYPE_TRANSMIT,0);
  energest_type_set(ENERGEST_TYPE_LISTEN,0);
  energest_type_set(ENERGEST_TYPE_IRQ,0);

  set_energy(INDEX_ENERGYCOST    , array , cpu         )      ;
  set_energy(INDEX_ENERGYCOST+6  , array , lpm         )      ;
  set_energy(INDEX_ENERGYCOST+12 , array , transmit)      ;
  set_energy(INDEX_ENERGYCOST+18 , array , listen   )      ;

  set_energy(INDEX_IRQ  , array , irq )      ;

  array[INDEX_PARENT] = 0;
  array[INDEX_PARENT+1] = 0;

  // //采样电压 
  //hitmac need to compensate one tick
  temp_votlage = 3600;//3600 get_voltage();
  array[INDEX_ADCVOLTAGE]       = (temp_votlage>>8)&0xff; 
  array[INDEX_ADCVOLTAGE+1]     = temp_votlage&0xff;

  //change to sync tick
  /*record hitmac sync diff tick numbers*/
  temp_sync_diff =get_sync_difftick()/2;//get_sync_difftick()/2;
  array[INDEX_BEACON_INTERVAL]  = (uint8_t)(temp_sync_diff>>24)&0xFF;        
  array[INDEX_BEACON_INTERVAL+1] = (uint8_t)(temp_sync_diff>>16)&0xFF;
  array[INDEX_BEACON_INTERVAL+2] = (uint8_t)(temp_sync_diff>>8)&0xFF;         
  array[INDEX_BEACON_INTERVAL+3] = (uint8_t)(temp_sync_diff)&0xFF;
  
  //change to receive rssi
  array[INDEX_RTMETRIC]         = 0xB0;        
  //change to surrounding noise 
  radio_value_t local_rssi;
  NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI,&local_rssi);//get local rssi
  array[INDEX_NUM_NEIGHBORS]        = (uint8_t)(local_rssi)&0xFF;
  //reboot number
  array[INDEX_RESTART_COUNT] = restart_count;
  //change to channel number
  radio_value_t local_channel;
  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL,&local_channel);//get local channel
  array[INDEX_TIME_DIFF] = (uint8_t)(local_channel)&0xFF;
 


#if DEBUG   

    static int i ; 
    i=0;

    PRINTF("\ntime ");
    while (i<INDEX_TOPO)
    {
      PRINTF(":%d",array[i]);
      i++;
    }

    PRINTF("\ntopo ");
    while (i<INDEX_ENERGYCOST)
    {
      PRINTF(":%x",array[i]);
      i++;
    }

    PRINTF("\nenergy ");
    while (i<INDEX_ADCVOLTAGE)
    {
      PRINTF("%02x ",array[i]);
      i++;
    }
      PRINTF("\nvoltage ");
    while (i<INDEX_BEACON_INTERVAL)
    {
      PRINTF("%02x ",array[i]);
      i++;
    }
    PRINTF("\ninterval ");
       while (i<INDEX_NUM_NEIGHBORS)
    {
      PRINTF("%02x ",array[i]);
      i++;
    }
      PRINTF("\nnum_neighbors ");
       while (i<INDEX_RTMETRIC)
    {
      PRINTF("%02x ",array[i]);
      i++;
    }
      PRINTF("\n rtmetric");
       while (i<INDEX_TIME_DIFF)
    {
      PRINTF("%02x ",array[i]);
      i++;
    }
      PRINTF("\n time-diff");
       while (i<INDEX_TIME_DIFF+1)
    {
      PRINTF("%02x ",array[i]);
      i++;
    }
    PRINTF("\n");
      PRINTF("\n restart count:");
    while (i<INDEX_RESTART_COUNT+1)
    {
      PRINTF("%02x ",array[i]);
      i++;
    }
    PRINTF("\n");

#endif 

}
/*---------------------------------------------------------------------------*/