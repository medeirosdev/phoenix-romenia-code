#ifndef __CONTROLLERS_H__
#define __CONTROLLERS_H__

#include "utils.h"

// Ponto de partida pros ganhos do PID - mesma ordem de grandeza do que
// funcionava no projeto antigo, mas isso PRECISA de ajuste fino na pista
// real (o chassi, o peso e os motores são diferentes).
#define LINE_PID_SETPOINT          0.0
#define LINE_PID_KP                1.5
#define LINE_PID_KI                0.0
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

#endif /* __CONTROLLERS_H__ */
