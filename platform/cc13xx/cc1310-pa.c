#include "cc1310-pa.h"
#include "ti-lib.h"
#include "contiki-conf.h"
#include "board.h"
/*---------------------------------------------------------------------------*/
void
pa_init()
{
#if PA_434MHZ
  ti_lib_rom_ioc_pin_type_gpio_output(CC1310_PA_CSD);
  ti_lib_gpio_write_dio(CC1310_PA_CSD, 0);

  ti_lib_rom_ioc_pin_type_gpio_output(CC1310_PA_CTX);
  ti_lib_gpio_write_dio(CC1310_PA_CTX, 0);
#endif
}
/*---------------------------------------------------------------------------*/
void
pa_mode(int mode)
{
#if PA_434MHZ
  if(mode == PA_SLEEP) {
    ti_lib_gpio_write_dio(CC1310_PA_CSD, 0);
  } else if(mode == PA_TX) {
    ti_lib_gpio_write_dio(CC1310_PA_CSD, 1);
    ti_lib_gpio_write_dio(CC1310_PA_CTX, 1);
  } else if(mode == PA_RX) {
    ti_lib_gpio_write_dio(CC1310_PA_CTX, 0);
  }
#endif
}
/*---------------------------------------------------------------------------*/