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
    float current_battery_voltage = get_battery_voltage();
    int16_t pwm_to_fan = round(255 * voltage_to_fan / current_battery_voltage);
    pwm_to_fan = constrain(pwm_to_fan, 0, 255);

    analogWrite(FAN_MOTOR_PIN, pwm_to_fan);
}

void fan_init() {
    pinMode(FAN_MOTOR_PIN, OUTPUT);
    analogWriteFrequency(50000);
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
