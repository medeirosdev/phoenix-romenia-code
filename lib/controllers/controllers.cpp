#include "controllers.h"
#include "line_sensors.h"
#include "motors.h"
#include "config.h"
#include "AD7490.h"

LinePIDController line_pid;

// Medicao de tempo da tentativa atual (zerada em init()) - ver
// pid_timing_report().
static uint32_t timing_samples = 0;
static uint32_t read_time_sum_us = 0;
static uint32_t read_time_max_us = 0;
static uint32_t period_samples = 0;
static uint32_t period_sum_us = 0;
static uint32_t period_max_us = 0;

// Valores em RAM que LinePIDController::init() realmente le - comecam
// iguais aos #define (controllers.h), e so mudam se um comando KP/KI/KD/MV
// chegar por Bluetooth/Serial (state_machine.cpp - handle_param_command()).
// Ficam valendo em toda tentativa (todo ST chama init() de novo) ate PR ou
// o ESP32 desligar - ver comentario em controllers.h.
static float override_kp = LINE_PID_KP;
static float override_ki = LINE_PID_KI;
static float override_kd = LINE_PID_KD;
static float override_motor_voltage = LINE_PID_BASE_VOLTAGE;

void set_pid_kp(float value) { override_kp = value; }
void set_pid_ki(float value) { override_ki = value; }
void set_pid_kd(float value) { override_kd = value; }
void set_motor_base_voltage(float value) { override_motor_voltage = value; }

void reset_pid_overrides() {
    override_kp = LINE_PID_KP;
    override_ki = LINE_PID_KI;
    override_kd = LINE_PID_KD;
    override_motor_voltage = LINE_PID_BASE_VOLTAGE;
}

float get_pid_kp() { return override_kp; }
float get_pid_kd() { return override_kd; }
float get_motor_base_voltage() { return override_motor_voltage; }

void LinePIDController::init() {
    setpoint = LINE_PID_SETPOINT;
    kP = override_kp;
    kI = override_ki;
    kD = override_kd;
    sampling_rate_ms = LINE_PID_SAMPLING_RATE_MS;
    motor_base_value = override_motor_voltage;
    current_error = 0;
    last_error = 0;
    accumulated_error = 0;
    last_sample_time_us = 0;
    race_start_ms = millis();
    has_previous_sample = false;
    filtered_derivative = 0;

    timing_samples = read_time_sum_us = read_time_max_us = 0;
    period_samples = period_sum_us = period_max_us = 0;
    reset_AD7490_channel_errors();
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
    if (has_previous_sample && now - last_sample_time_us < (unsigned long)(sampling_rate_ms * 1000)) return;

    // Tempo REAL desde a amostra anterior (nao o sampling_rate_ms fixo): se
    // o loop atrasar, o D continua certo.
    unsigned long period_us = now - last_sample_time_us;
    last_sample_time_us = now;

    current_error = setpoint - read_robot_position();
    unsigned long read_time_us = micros() - now;

    timing_samples++;
    read_time_sum_us += read_time_us;
    read_time_max_us = max(read_time_max_us, (uint32_t)read_time_us);

    if (has_previous_sample) {
        period_samples++;
        period_sum_us += period_us;
        period_max_us = max(period_max_us, (uint32_t)period_us);

        // Mesma unidade de antes (erro por segundo), entao o KD continua
        // valendo na mesma escala - so com tempo real e filtrado.
        double raw_derivative = (current_error - last_error) / (period_us / 1e6);
        filtered_derivative += LINE_PID_D_FILTER_ALPHA * (raw_derivative - filtered_derivative);
    }
    has_previous_sample = true;
    accumulated_error += current_error;

    double correction = (kP * current_error)
                       + (kD * filtered_derivative)
                       + (kI * accumulated_error);

    last_error = current_error;

    double base = motor_base_value;
    if (PASSO_MOTOR_LIGADO) {
        double elapsed_s = (millis() - race_start_ms) / 1000.0;
        base = min(motor_base_value, TAMANHO_PASSO * elapsed_s);
    }

    set_motor_voltage(RIGHT_MOTOR, base + correction);
    set_motor_voltage(LEFT_MOTOR, base - correction);
}

void controllers_init() {
    line_pid.init();
}

String pid_timing_report() {
    if (timing_samples == 0) return "";

    String report = "PID: leitura media " + String(read_time_sum_us / timing_samples)
                  + "us max " + String(read_time_max_us) + "us";
    if (period_samples > 0) {
        report += " | periodo medio " + String(period_sum_us / period_samples)
                + "us max " + String(period_max_us) + "us";
    }
    report += " | amostras " + String(timing_samples)
            + " | erros canal ADC " + String(get_AD7490_channel_errors());

    // Consome o relatorio: uma parada sem PID rodando depois (ex.: SP
    // durante a subida da turbina) nao repete os numeros da tentativa velha.
    timing_samples = 0;
    period_samples = 0;
    return report;
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
