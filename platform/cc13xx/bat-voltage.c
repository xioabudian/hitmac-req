#include "bat-voltage.h"

int
get_voltage()     /* uint mV */
{
  int voltage = 0;

  ti_lib_aon_batmon_enable();

  for(int i = 0; i < 50; i++) { /* 200*200 200*100 100*100 '50*100' available */
    for(int j = 0; j < 100; j++) { /* 25*100 isn't available */
      __asm("nop");
    }
  }
  voltage = (int)ti_lib_aon_batmon_battery_voltage_get();

  /* return  int */
  /* [10:8] represents INT */
  /* [7:0]  represents FRAC */
  /* each uint shows 0.00390625v */

  voltage = (voltage >> 8) * 1000 + (voltage & 0xFF) * 1000 / 256;

  ti_lib_aon_batmon_disable();

  return voltage;
}