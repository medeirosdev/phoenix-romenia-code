#ifndef __MOTORS_H__
#define __MOTORS_H__

#include "utils.h"

// Teto de seguranca pra tensao ENVIADA ao motor - nao precisa ser igual a
// tensao da bateria (que agora pode chegar a 16,8V com pack 4S, ver
// battery.h). Ajustar esse valor e uma escolha de "velocidade maxima",
// nao uma calibracao obrigatoria.
#define MAX_MOTOR_VOLTAGE  12.0
#define PWM_FREQUENCY_HZ   50000 // 50 kHz

// Ainda nao sabemos qual motor (1 ou 2) fica fisicamente a esquerda ou a
// direita no robo montado (pergunta em aberto, ver PLANEJAMENTO.md secao
// 12) - por isso os nomes ficam MOTOR_1/MOTOR_2 por enquanto, nao
// LEFT_MOTOR/RIGHT_MOTOR. Ajustar aqui quando isso for confirmado.
enum Motor_Id {
    MOTOR_1,
    MOTOR_2
};

void motors_init();
void set_motor_voltage(Motor_Id motor, float voltage_to_motor);
void brake_motors(bool active_brake);
void validar_motores();

#endif /* __MOTORS_H__ */
