#include "controllers.h"
#include "line_sensors.h"
#include "motors.h"

LinePIDController line_pid;

void LinePIDController::init() {
    setpoint = LINE_PID_SETPOINT;
    kP = LINE_PID_KP;
    kI = LINE_PID_KI;
    kD = LINE_PID_KD;
    sampling_rate_ms = LINE_PID_SAMPLING_RATE_MS;
    motor_base_value = LINE_PID_BASE_VOLTAGE;
    current_error = 0;
    last_error = 0;
    accumulated_error = 0;
    last_sample_time_us = 0;
}

// Le a posicao do robo em relacao a linha e aplica a correcao nos 2
// motores. So roda de fato a cada sampling_rate_ms - chamar mais rapido
// que isso nao adianta, o AD7490 e o gargalo de velocidade, nao o loop.
//
// ATENCAO: qual motor recebe +correcao e qual recebe -correcao depende
// de qual e fisicamente esquerdo/direito - isso ainda NAO foi confirmado
// (PLANEJAMENTO.md secao 17, vai ser validado na bancada). Se o robo
// curvar pro lado errado quando a linha sai do centro, e so inverter os
// dois analogWrite abaixo (trocar MOTOR_1 por MOTOR_2).
void LinePIDController::run() {
    unsigned long now = micros();
    if (now - last_sample_time_us < (unsigned long)(sampling_rate_ms * 1000)) return;
    last_sample_time_us = now;

    current_error = setpoint - read_robot_position();
    double delta_error = current_error - last_error;
    accumulated_error += current_error;

    double correction = (kP * current_error)
                       + (kD * (delta_error * 1000.0) / sampling_rate_ms)
                       + (kI * accumulated_error);

    last_error = current_error;

    set_motor_voltage(MOTOR_1, motor_base_value + correction);
    set_motor_voltage(MOTOR_2, motor_base_value - correction);
}

void controllers_init() {
    line_pid.init();
}

// A "validacao" desse modulo e, na pratica, o objetivo inteiro do
// firmware nesse estagio: seguir a linha. Diferente dos outros
// validar_*(), essa funcao NUNCA RETORNA (fica rodando o PID pra
// sempre) - vai ser substituida quando o state_machine existir
// (PLANEJAMENTO.md secao 13, passo 6), que vai dar um jeito de verdade
// de iniciar/parar a corrida.
//
// Calibra os sensores frontais primeiro (5s), depois entra no loop do
// PID. IMPORTANTE: colocar o robo NA LINHA antes de ligar, com espaco
// livre na pista - nesse estagio ainda nao existe comando de parada,
// só desligando a energia.
void validar_controllers() {
    Serial.println("[validar_controllers] Calibrando sensores frontais...");
    calibrate_line_sensors();

    Serial.println("[validar_controllers] Seguindo linha - sem comando de parada ainda, desligue a energia pra parar");
    controllers_init();

    while (true) {
        line_pid.run();
    }
}
