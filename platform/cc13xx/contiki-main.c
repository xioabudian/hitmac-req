#include "ti-lib.h"
#include "contiki.h"
#include "sys_ctrl.h"
#include "dev/oscillators.h"
#include "dev/soc-rtc.h"
#include "dev/watchdog.h"
#include "dev/uart1.h"
#include "dev/serial-line.h"
#include "lib/sensors.h"
#include "lpm.h"
/* Net Setting */
#include "node-id.h"
#include "ieee-addr.h"
#include "contiki-net.h"
#include "net/netstack.h"
#include "rf-core/rf-core.h"

/************************************/
#include "external-watchdog.h"

#include <stdio.h>
#include <string.h>
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
/** \brief Board specific iniatialisation */
void board_init(void);
/*---------------------------------------------------------------------------*/
void set_nodeid(){
  uint16_t short_addr;
  uint8_t ext_addr[8];
  ieee_addr_cpy_to(ext_addr, 8);

  short_addr = ext_addr[7];
  short_addr |= ext_addr[6] << 8;
  node_id_burn(short_addr);
  /****************set node-id**********************/
  printf("node_id: %x\n", node_id);
}
/*---------------------------------------------------------------------------*/
static void
set_rf_params(void)
{
  uint16_t short_addr;
  uint8_t ext_addr[8];
  radio_value_t val = 0;

  ieee_addr_cpy_to(ext_addr, 8);

  short_addr = ext_addr[7];
  short_addr |= ext_addr[6] << 8;

  /* Populate linkaddr_node_addr. Maintain endianness */
  memcpy(&linkaddr_node_addr, &ext_addr[8 - LINKADDR_SIZE], LINKADDR_SIZE);
  
  
  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
  //set channel
  normalbyte_rfchannel_restore();
  if(channel_byte<HITMAC_MAX_CHANNEL){
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel_byte);
  }else{
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, RF_CORE_CHANNEL);
  }

  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, 8);

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &val);

  printf("channel: %u\n", val);
}
/*---------------------------------------------------------------------------*/
int
main(void)
{
  /* Enable flash cache and prefetch. */
  ti_lib_vims_mode_set(VIMS_BASE, VIMS_MODE_ENABLED);
  ti_lib_vims_configure(VIMS_BASE, true, true);

  ti_lib_int_master_disable();
  /* Set the LF XOSC as the LF system clock source */
  oscillators_select_lf_xosc();

  lpm_init();

  board_init();

  printf("Starting " CONTIKI_VERSION_STRING "\n");
  printf("supports cc13xx platform\n");
  printf("author HIT-CPSGroup\n");

  /**********************************************/
#if 0

  restart_count_byte_burn(0);

#else

  restart_count_byte_restore();
  restart_count++;
  restart_count_byte_burn(restart_count);
  printf("restart count %d\n", restart_count);
#endif /* BURN_NODEID */

  /*************************************************/

  ti_lib_int_master_enable();

  soc_rtc_init();

  clock_init(); /* radio combine timer */

  rtimer_init();

  watchdog_init();

  process_init();

  random_init(0x1234);

#if CC26XX_UART_CONF_ENABLE
  cc26xx_uart_init();
#endif

  serial_line_init();
  
  //input_uart_data,this bug will  lead nodes restart
  // uart1_set_input(serial_line_input_byte);

  process_start(&etimer_process, NULL);
  ctimer_init();

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  printf(" Net: ");
  printf("%s\n", NETSTACK_NETWORK.name);
  printf(" MAC: ");
  printf("%s\n", NETSTACK_MAC.name);
  printf(" RDC: ");
  printf("%s\n", NETSTACK_RDC.name);

  set_nodeid();

  netstack_init();

  set_rf_params();

#if EXTER_WATCHDOG
  process_start(&watchdog_process,NULL);
#endif
  autostart_start(autostart_processes);

  watchdog_start();

  while(1) {

    uint8_t r;

    do {
      r = process_run();
      watchdog_periodic();
    } while(r > 0);

#if !ROOTNODE
    lpm_drop();
#endif
  }
}