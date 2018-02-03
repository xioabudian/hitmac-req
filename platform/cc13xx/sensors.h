#ifndef SENSOR_H_
#define SENSOR_H_
/*---------------------------------------------------------------------------*/
#include "lib/sensors.h"
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define BUTTON_SENSOR "Button"
/*---------------------------------------------------------------------------*/
#define BUTTON_SENSOR_VALUE_STATE    0
#define BUTTON_SENSOR_VALUE_DURATION 1

#define BUTTON_SENSOR_VALUE_RELEASED 0
#define BUTTON_SENSOR_VALUE_PRESSED  1
/*---------------------------------------------------------------------------*/
extern const struct sensors_sensor button_select_sensor;
extern const struct sensors_sensor button_left_sensor;
extern const struct sensors_sensor button_right_sensor;
extern const struct sensors_sensor button_up_sensor;
extern const struct sensors_sensor button_down_sensor;
/*---------------------------------------------------------------------------*/
#endif /* SENSOR_H_ */