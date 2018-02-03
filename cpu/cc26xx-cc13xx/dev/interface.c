#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "dev/interface.h"
#include "net/packetbuf.h"

#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/leds.h"
static unsigned char *original_dataptr;

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
  /* printf("length: %d\n",len); */
  /* printf("%s\n",original_dataptr); */
   // if(len==3){ 
  /*  printf("seq: %d\n",original_dataptr[2]); */
  /* } */
  leds_toggle(LEDS_RED);
  printf("%s  ", original_dataptr);

  printf("receive rssi %d\n", (int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI));
  printf("receive len %d",len);
  /* char ack[6]="RXACK"; */
  /* if(filter(packetbuf_dataptr(),len)){ */
  /* printf("%s\n",original_dataptr); */
  /*  NETSTACK_RADIO.send(ack,sizeof(ack)); */

  /* } */
}
/*---------------------------------------------------------------------------*/