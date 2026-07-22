#include "fan.h"
#include "pinout.h"
#include "battery.h"

// Turbina de succao: motor de 1 sentido so (nao precisa reverter), com um
// UNICO pino de PWM - diferente dos motores de tracao, que tem par
// IN1/IN2 (ver PLANEJAMENTO.md secao 4.3). Por isso o duty cycle aqui e
// escrito direto (quanto maior o PWM, mais forte a turbina).
//
// ATENCAO: essa relacao (PWM alto = turbina mais forte, sem inversao) e
// uma suposicao razoavel pra um driver de 1 pino, mas ainda NAO foi
// confirmada no hardware real - conferir na bancada se o sentido bate
// antes de confiar nesses valores em corrida.
void set_fan_voltage(float voltage_to_fan) {
    voltage_to_fan = constrain(voltage_to_fan, 0, MAX_FAN_VOLTAGE);
    // Piso de seguranca: mesma razao do motors.cpp - evita divisao por
    // quase-zero se a bateria estiver desconectada/lendo errado.
    float current_battery_voltage = max(get_battery_voltage(), (float)MIN_BATTERY_VOLTAGE_FOR_PWM);
    int16_t pwm_to_fan = round(255 * voltage_to_fan / current_battery_voltage);
    pwm_to_fan = constrain(pwm_to_fan, 0, 255);

    analogWrite(FAN_MOTOR_PIN, pwm_to_fan);
}

void fan_init() {
    pinMode(FAN_MOTOR_PIN, OUTPUT);
    // Essa versao do core ESP32 so tem analogWriteFrequency()/
    // analogWriteResolution() GLOBAIS (sem variante por pino) - ver
    // comentario completo em motors.cpp (motors_init). Por isso o valor
    // aqui PRECISA continuar igual ao usado em motors.cpp (50 kHz, 8 bits).
    analogWriteFrequency(FAN_PWM_FREQUENCY_HZ);
    analogWriteResolution(8);

    set_fan_voltage(0);
}

// Validacao de bancada: liga a turbina em potencia baixa por 2s, desliga,
// espera 2s. Testar com a turbina longe de fios soltos/dedos/cabelo.
void validar_fan() {
    Serial.println("[validar_fan] ligando turbina (5V equivalente)");
    set_fan_voltage(5.0);
    delay(2000);

    Serial.println("[validar_fan] desligando turbina");
    set_fan_voltage(0);
    delay(2000);

    Serial.println("[validar_fan] fim");
}
