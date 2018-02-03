#include "contiki.h"
#include "ti-lib.h"
#include "external-watchdog.h"
PROCESS(watchdog_process, "External Watchdog Process");
static struct etimer dog;
#define WAT_PERIOD (CLOCK_SECOND*60*4)
/*---------------------------------------------------------------------------*/
//@test
#include "dev/leds.h"
void logic_test(uint32_t i);
/*---------------------------------------------------------------------------*/
void
external_watch_dog(int tick)      /* by xiaobing,external watch dog DIO14 */
{
  ti_lib_gpio_write_dio(BOARD_IOID_DIO14, 1);
  // logic_test(1);
  /* delay 10us */
  for(int i = 0; i < tick; i++) {
    for(int j = 0; j < i; ++j) {
      __asm("nop");
    }
  }
  // logic_test(0);
  ti_lib_gpio_write_dio(BOARD_IOID_DIO14, 0);
  
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(watchdog_process,ev,data)
{
	 PROCESS_BEGIN();
	 etimer_set(&dog,WAT_PERIOD);

	 while(1){
	 	PROCESS_YIELD();
	 	if(etimer_expired(&dog) && ev == PROCESS_EVENT_TIMER){
	 		/*periodically feeding watchdog*/
	 		external_watch_dog(15);
	 	}
	 	etimer_set(&dog,WAT_PERIOD);
	 }

	 PROCESS_END();
}

/*---------------------------------------------------------------------------*/