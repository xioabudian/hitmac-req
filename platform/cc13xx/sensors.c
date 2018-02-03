#include "contiki.h"
#include "sensors.h"

#include <string.h>
/*---------------------------------------------------------------------------*/
/** \brief Exports a global symbol to be used by the sensor API */
SENSORS(&button_select_sensor, &button_left_sensor, &button_right_sensor,
        &button_up_sensor, &button_down_sensor);
/*---------------------------------------------------------------------------*/
/** @} */
