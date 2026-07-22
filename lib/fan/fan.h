#ifndef __FAN_H__
#define __FAN_H__

#include "utils.h"

#define MAX_FAN_VOLTAGE 12.0

void fan_init();
void set_fan_voltage(float voltage_to_fan);
void validar_fan();

#endif /* __FAN_H__ */
