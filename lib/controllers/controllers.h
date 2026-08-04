#ifndef __CONTROLLERS_H__
#define __CONTROLLERS_H__

#include "utils.h"

// Ponto de partida pros ganhos do PID - mesma ordem de grandeza do que
// funcionava no projeto antigo, mas isso PRECISA de ajuste fino na pista
// real (o chassi, o peso e os motores são diferentes).
#define LINE_PID_SETPOINT          0.0
#define LINE_PID_KP                1.5
#define LINE_PID_KI                0.0  // ATENCAO: com kI > 0, accumulated_error (controllers.cpp)
                                          // nao tem limite (sem anti-windup) - se for usar kI de
                                          // verdade, adicionar um clamp em accumulated_error antes.
                                          // Inofensivo hoje porque kI=0 zera o termo de qualquer jeito.
#define LINE_PID_KD                0.015
#define LINE_PID_SAMPLING_RATE_MS  2.0

// Tensao base dos motores durante esse teste - baixa de proposito, pra
// ser seguro na primeira vez que o robo roda sozinho numa pista.
#define LINE_PID_BASE_VOLTAGE 1.2

class LinePIDController {
    public:
        double setpoint;
        double kP, kI, kD;
        double motor_base_value; // tensao base (V) somada/subtraida da correcao
        uint16_t sampling_rate_ms;

        void init();
        void run();

    private:
        double current_error = 0;
        double last_error = 0;
        double accumulated_error = 0;
        unsigned long last_sample_time_us = 0;
};

extern LinePIDController line_pid;

void controllers_init();
void validar_controllers();

// Sobrescrevem, em RAM, os ganhos do PID e a tensao base do motor pro
// PROXIMO ST (controllers_init() le esses valores, nao mais direto os
// #define acima) - pensado pra testar ajuste sem recompilar/regravar.
// Comandos correspondentes (KP/KI/KD/MV) em state_machine.cpp. Nao grava
// em EEPROM de proposito: sao valores de teste de bancada, somem se o
// ESP32 desligar - use PR (reset_pid_overrides) pra voltar ao padrao do
// codigo sem precisar disso.
void set_pid_kp(float value);
void set_pid_ki(float value);
void set_pid_kd(float value);
void set_motor_base_voltage(float value);
void reset_pid_overrides();

#endif /* __CONTROLLERS_H__ */
