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
// Convencao (mesma do projeto antigo): posicao positiva = linha pra
// direita -> erro negativo -> correcao negativa -> motor direito freia
// (RIGHT_MOTOR = base + correcao) e motor esquerdo acelera (LEFT_MOTOR =
// base - correcao), o que vira o robo pra direita, de volta pro centro.
//
// Isso assume que o sensor 0 do AD7490 e fisicamente o mais a esquerda
// (ainda nao verificado na bancada, ver nota em AD7490.cpp sobre a
// reordenacao dos 4 primeiros canais). Se o robo curvar pro lado errado,
// o problema mais provavel e essa suposicao, nao esse calculo aqui.
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

    set_motor_voltage(RIGHT_MOTOR, motor_base_value + correction);
    set_motor_voltage(LEFT_MOTOR, motor_base_value - correction);
}

void controllers_init() {
    line_pid.init();
}

// Teste isolado do PID, fora da maquina de estados (que ja existe em
// state_machine.cpp, com calibracao/start/stop de verdade via comando -
// prefira usar aquele fluxo). Essa funcao NUNCA RETORNA (fica rodando o
// PID pra sempre) - util só pra testar o PID sozinho, sem depender de
// nenhum comando externo.
//
// Calibra os sensores frontais primeiro (5s), depois entra no loop do
// PID. IMPORTANTE: colocar o robo NA LINHA antes de ligar, com espaco
// livre na pista - aqui nao tem comando de parada, só desligando a
// energia.
void validar_controllers() {
    Serial.println("[validar_controllers] Calibrando sensores frontais...");
    calibrate_line_sensors();

    Serial.println("[validar_controllers] Seguindo linha - sem comando de parada ainda, desligue a energia pra parar");
    controllers_init();

    while (true) {
        line_pid.run();
    }
}
