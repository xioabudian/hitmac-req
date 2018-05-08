#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "dev/interface.h"
#include "net/packetbuf.h"

#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/leds.h"
#include "IMP.h"
static unsigned char *original_dataptr;
void logic_test(uint32_t i);
/*---------------------------------------------------------------------------*/
bool
filter(unsigned char *p, int len)
{

  bool flag = true;
  char *cmp = "Hello";
  if(len < 5) {
    return false;
  }

  len = len > 5 ? 5 : len;
  for(int i = 0; i < len; i++) {
    if(cmp[i] != p[i]) {
      flag = false;
      break;
    }
  }

  return flag;
}
/*---------------------------------------------------------------------------*/
void
packet_input_test(int len)
{
  original_dataptr = packetbuf_dataptr();
  
  leds_toggle(LEDS_RED);

  #define ACK_LEN 3
  uint8_t ackbuf[ACK_LEN] = {0,0,3};
  NETSTACK_RADIO.send(ackbuf,sizeof(ackbuf));

  printf("%s  ", original_dataptr);

  // printf("receive rssi %d\n", (int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));

  // printf("receive len %d",len);

  // clear_display(0xFF);
  // print_key_value("receive rssi:",(int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
  // 
  // for(int i=0;i<len;i++){
  //   buf[i] = *original_dataptr;
  //   original_dataptr++;
  // }
  // buf[len]='\0';
  // print_key_value(buf,255);
  // // printf("receive rssi %d", (int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
  // // clear_display(0xFF);
  // print_key_value("receive rssi:",(int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));

  
}
/*---------------------------------------------------------------------------*/