#ifndef __MOTORS_H__
#define __MOTORS_H__

#include "utils.h"

// Teto de seguranca pra tensao ENVIADA ao motor - nao precisa ser igual a
// tensao da bateria (que agora pode chegar a 16,8V com pack 4S, ver
// battery.h). Ajustar esse valor e uma escolha de "velocidade maxima",
// nao uma calibracao obrigatoria.
#define MAX_MOTOR_VOLTAGE  12.0
#define PWM_FREQUENCY_HZ   50000 // 50 kHz

// Esquerda/direita confirmados no documento de referencia de pinout
// (22/07/2026) - ver pinout.h.
enum Motor_Id {
    LEFT_MOTOR,
    RIGHT_MOTOR
};

void motors_init();
void set_motor_voltage(Motor_Id motor, float voltage_to_motor);
void brake_motors(bool active_brake);
void validar_motores();

#endif /* __MOTORS_H__ */
