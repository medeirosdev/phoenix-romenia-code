#ifndef __FAN_H__
#define __FAN_H__

#include "utils.h"

#define MAX_FAN_VOLTAGE     12.0
#define FAN_PWM_FREQUENCY_HZ 50000 // 50 kHz - mesmo valor usado nos motores de tracao,
                                    // mas com macro propria (nao depende de motors.h)

void fan_init();
void set_fan_voltage(float voltage_to_fan);
// Escreve o duty cycle (0-255) DIRETO no pino, sem nenhum calculo baseado
// na tensao da bateria - decisao do usuario/Ricardo, 03/08/2026: enquanto
// a calibracao de bateria (battery.h) nao estiver confiavel, e mais
// previsivel escolher o PWM a mao do que confiar num valor de tensao
// convertido por uma formula que hoje da resultado errado (ver
// set_fan_voltage(), que multiplicou 3V pedidos em ~9V reais no teste de
// bancada).
void set_fan_pwm_manual(uint8_t pwm);
void validar_fan();

#endif /* __FAN_H__ */
