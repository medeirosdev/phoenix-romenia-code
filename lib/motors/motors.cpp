#include "motors.h"
#include "pinout.h"
#include "battery.h"

// =====================================================================
// Driver dos motores: BTN9960LV (Infineon) - confirmado por Ricardo,
// 21/07/2026 (datasheet enviado no WhatsApp). Ver PLANEJAMENTO.md
// secao 4.7 pra explicacao completa.
//
// ESSE CHIP E UMA MEIA-PONTE SIMPLES (1 MOSFET high-side + 1 low-side),
// nao um H-bridge integrado como no projeto antigo (bia-senna-code-2026).
// Cada motor usa 2 chips independentes, um por terminal - por isso
// PWM A/B (Motor 1) e PWM C/D (Motor 2) sao 2 sinais SEPARADOS, um por
// chip, nao um par "PWM + direcao" como antes.
//
// Tabela-verdade do datasheet (pino IN, com INH=1/habilitado):
//   IN=0 -> terminal no GND (lado baixo ativo)
//   IN=1 -> terminal na tensao da bateria (lado alto ativo)
// Ou seja, com INH fixo em habilitado, o proprio pino IN funciona como
// um PWM comum (0-255 = 0%-100% do tempo no lado alto).
//
// SUPOSICAO AINDA NAO CONFIRMADA: assumimos que o pino INH de cada chip
// esta FIXO/habilitado na placa (nao controlado pelo ESP32), porque so
// existe 1 sinal rotulado por chip no schematic (PWM A, nao "PWM A" +
// "INH A"). Se isso estiver errado - se o INH tambem for controlado pelo
// ESP32 e estiver flutuando/desabilitado - os motores simplesmente nao
// vao girar (o datasheet diz que INH tem pull-down interno, ou seja,
// default = desabilitado se nao for ligado em nada). Essa suposicao esta
// registrada como pergunta pro Ricardo (PLANEJAMENTO.md secao 17).
//
// Esquema pra driver bidirecional com 2 meias-pontes independentes
// (1 por terminal do motor), tambem chamado de "sign-magnitude":
//   frente:  terminal A = PWM(duty)     | terminal B = 0
//   re:      terminal A = 0             | terminal B = PWM(duty)
//   parado:  terminal A = 0             | terminal B = 0  (os dois no
//            GND = freio, ja que com INH sempre habilitado nao existe
//            um estado "roda livre"/tri-state disponivel via IN sozinho)
// =====================================================================

// Pino de cada terminal do motor (motor_terminal_a = chip A, motor_terminal_b = chip B).
static const uint8_t motor_terminal_a_pins[] = {MOTOR_1_IN1_PIN, MOTOR_2_IN1_PIN};
static const uint8_t motor_terminal_b_pins[] = {MOTOR_1_IN2_PIN, MOTOR_2_IN2_PIN};

// Escreve uma tensao (-MAX..+MAX) num motor, convertendo pra PWM (0-255)
// proporcional a tensao ATUAL da bateria - assim o motor recebe a mesma
// "forca" mesmo com a bateria descarregando ao longo da corrida.
void set_motor_voltage(Motor_Id motor, float voltage_to_motor) {
    voltage_to_motor = constrain(voltage_to_motor, -MAX_MOTOR_VOLTAGE, MAX_MOTOR_VOLTAGE);
    // Piso de seguranca: sem isso, bateria desconectada/lendo perto de 0
    // (ex.: testando na bancada so com USB) causa divisao por quase-zero
    // logo abaixo, o que e comportamento indefinido ao converter pra int16.
    float current_battery_voltage = max(get_battery_voltage(), (float)MIN_BATTERY_VOLTAGE_FOR_PWM);
    int16_t pwm_to_motor = round(255 * voltage_to_motor / current_battery_voltage);
    pwm_to_motor = constrain(pwm_to_motor, -255, 255);

    if (pwm_to_motor > 0) {
        analogWrite(motor_terminal_a_pins[motor], pwm_to_motor);
        analogWrite(motor_terminal_b_pins[motor], 0);
    } else if (pwm_to_motor < 0) {
        analogWrite(motor_terminal_a_pins[motor], 0);
        analogWrite(motor_terminal_b_pins[motor], -pwm_to_motor);
    } else {
        analogWrite(motor_terminal_a_pins[motor], 0);
        analogWrite(motor_terminal_b_pins[motor], 0);
    }
}

void motors_init() {
    for (uint8_t motor = 0; motor < 2; motor++) {
        pinMode(motor_terminal_a_pins[motor], OUTPUT);
        pinMode(motor_terminal_b_pins[motor], OUTPUT);
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

// Freia os dois motores (os dois terminais no GND). Com esse driver (ver
// nota no topo do arquivo), nao existe opcao de "roda livre" via os
// pinos IN sozinhos - so temos esse unico estado parado, que ja e um
// freio (os dois lados baixos ativos). O parametro active_brake fica so
// pra manter a mesma assinatura de funcao do projeto antigo; nao muda o
// comportamento aqui.
void brake_motors(bool active_brake) {
    (void)active_brake; // sem efeito nesse driver - ver comentario acima
    for (uint8_t motor = 0; motor < 2; motor++) {
        analogWrite(motor_terminal_a_pins[motor], 0);
        analogWrite(motor_terminal_b_pins[motor], 0);
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
