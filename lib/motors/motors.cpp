#include "motors.h"
#include "pinout.h"
#include "battery.h"

// Pinos IN1/IN2 de cada motor, indexados pelo enum Motor_Id.
static const uint8_t motor_in1_pins[] = {MOTOR_1_IN1_PIN, MOTOR_2_IN1_PIN};
static const uint8_t motor_in2_pins[] = {MOTOR_1_IN2_PIN, MOTOR_2_IN2_PIN};

// Escreve uma tensao (-MAX..+MAX) num motor, convertendo pra PWM (0-255)
// proporcional a tensao ATUAL da bateria - assim o motor recebe a mesma
// "forca" mesmo com a bateria descarregando ao longo da corrida. Sinal
// negativo inverte o sentido de giro (ponte H em modo bidirecional).
void set_motor_voltage(Motor_Id motor, float voltage_to_motor) {
    voltage_to_motor = constrain(voltage_to_motor, -MAX_MOTOR_VOLTAGE, MAX_MOTOR_VOLTAGE);
    // Piso de seguranca: sem isso, bateria desconectada/lendo perto de 0
    // (ex.: testando na bancada so com USB) causa divisao por quase-zero
    // logo abaixo, o que e comportamento indefinido ao converter pra int16.
    float current_battery_voltage = max(get_battery_voltage(), (float)MIN_BATTERY_VOLTAGE_FOR_PWM);
    int16_t pwm_to_motor = round(255 * voltage_to_motor / current_battery_voltage);
    pwm_to_motor = constrain(pwm_to_motor, -255, 255);

    if (pwm_to_motor > 0) {
        analogWrite(motor_in1_pins[motor], 255 - pwm_to_motor);
        analogWrite(motor_in2_pins[motor], 255);
    } else if (pwm_to_motor < 0) {
        analogWrite(motor_in1_pins[motor], 255);
        analogWrite(motor_in2_pins[motor], 255 + pwm_to_motor);
    } else {
        analogWrite(motor_in1_pins[motor], 0);
        analogWrite(motor_in2_pins[motor], 0);
    }
}

void motors_init() {
    for (uint8_t motor = 0; motor < 2; motor++) {
        pinMode(motor_in1_pins[motor], OUTPUT);
        pinMode(motor_in2_pins[motor], OUTPUT);
    }

    // Essa versao do core ESP32 (Arduino) so tem a variante GLOBAL de
    // analogWriteFrequency()/analogWriteResolution() - nao existe uma por
    // pino aqui. Ou seja, TODO pino PWM do projeto (motores e turbina)
    // precisa usar a MESMA frequencia/resolucao, e quem chamar essa funcao
    // por ultimo antes do primeiro analogWrite() de cada pino "vence". Como
    // motors e fan usam o mesmo valor (PWM_FREQUENCY_HZ / FAN_PWM_FREQUENCY_HZ
    // = 50 kHz, 8 bits), isso nao causa problema hoje - mas se um dia
    // precisar de frequencias diferentes por peripherico, vai exigir usar
    // a API de LEDC diretamente (nao mais analogWrite).
    analogWriteFrequency(PWM_FREQUENCY_HZ);
    analogWriteResolution(8);

    set_motor_voltage(MOTOR_1, 0);
    set_motor_voltage(MOTOR_2, 0);
}

// Trava ativa (curto-circuita as duas pernas da ponte H, freia mais forte)
// ou so desliga o PWM (roda livre, para por atrito/inercia).
void brake_motors(bool active_brake) {
    uint8_t brake_pwm = active_brake ? 100 : 0;
    for (uint8_t motor = 0; motor < 2; motor++) {
        analogWrite(motor_in1_pins[motor], brake_pwm);
        analogWrite(motor_in2_pins[motor], brake_pwm);
    }
}

// Validacao de bancada: gira cada motor pra frente, pra tras, e para.
// IMPORTANTE: na primeira vez que testar um motor novo, deixar a roda
// LEVANTADA DO CHAO, pra nao sair correndo por engano.
void validar_motores() {
    Serial.println("[validar_motores] Motor 1 sentido horario");
    set_motor_voltage(MOTOR_1, 3.0);
    delay(1500);
    set_motor_voltage(MOTOR_1, 0);
    delay(500);

    Serial.println("[validar_motores] Motor 1 sentido anti-horario");
    set_motor_voltage(MOTOR_1, -3.0);
    delay(1500);
    set_motor_voltage(MOTOR_1, 0);
    delay(1000);

    Serial.println("[validar_motores] Motor 2 sentido horario");
    set_motor_voltage(MOTOR_2, 3.0);
    delay(1500);
    set_motor_voltage(MOTOR_2, 0);
    delay(500);

    Serial.println("[validar_motores] Motor 2 sentido anti-horario");
    set_motor_voltage(MOTOR_2, -3.0);
    delay(1500);
    set_motor_voltage(MOTOR_2, 0);

    Serial.println("[validar_motores] fim");
}
