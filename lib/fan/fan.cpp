#include "fan.h"
#include "pinout.h"
#include "battery.h"
#include "bluetooth.h"

// Turbina de succao: motor de 1 sentido so (nao precisa reverter), com um
// UNICO pino de PWM - diferente dos motores de tracao, que tem par
// IN1/IN2 (ver PLANEJAMENTO.md secao 4.3). Por isso o duty cycle aqui e
// escrito direto (quanto maior o PWM, mais forte a turbina).
//
// ATENCAO: essa relacao (PWM alto = turbina mais forte, sem inversao) e
// uma suposicao razoavel pra um driver de 1 pino, mas ainda NAO foi
// confirmada no hardware real - conferir na bancada se o sentido bate
// antes de confiar nesses valores em corrida.
// Guardados so pra validar_fan() poder mandar telemetria completa por
// Bluetooth (achado de teste de bancada, 03/08/2026: turbina ligando e
// desligando varias vezes rapido no meio de um teste - precisa desses
// valores no exato momento do problema pra diagnosticar).
static float last_requested_voltage = 0;
static int16_t last_pwm_to_fan = 0;

void set_fan_voltage(float voltage_to_fan) {
    voltage_to_fan = constrain(voltage_to_fan, 0, MAX_FAN_VOLTAGE);
    // Piso de seguranca: mesma razao do motors.cpp - evita divisao por
    // quase-zero se a bateria estiver desconectada/lendo errado.
    float current_battery_voltage = max(get_battery_voltage(), (float)MIN_BATTERY_VOLTAGE_FOR_PWM);
    int16_t pwm_to_fan = round(255 * voltage_to_fan / current_battery_voltage);
    pwm_to_fan = constrain(pwm_to_fan, 0, 255);

    last_requested_voltage = voltage_to_fan;
    last_pwm_to_fan = pwm_to_fan;

    analogWrite(FAN_MOTOR_PIN, pwm_to_fan);
}

// Escreve o duty cycle direto no pino, SEM passar pela conta de tensao/
// bateria de set_fan_voltage() - decisao do Ricardo, 03/08/2026: enquanto
// BATTERY_VOLTAGE_PARAMETER/BATTERY_ADC_PARAMETER (battery.h) nao
// estiverem calibrados de verdade, essa conta da resultado errado (teste
// de bancada: pedir 3V resultou em ~9V medidos no multimetro). Ate a
// calibracao de bateria ser fechada, e mais previsivel escolher o PWM a
// mao. last_requested_voltage fica em -1 pra telemetria (send_fan_telemetry)
// deixar claro que essa leitura nao se aplica em modo manual.
void set_fan_pwm_manual(uint8_t pwm) {
    last_requested_voltage = -1;
    last_pwm_to_fan = pwm;

    analogWrite(FAN_MOTOR_PIN, pwm);
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

// Manda uma linha de telemetria completa (Serial + Bluetooth) com tudo que
// pode explicar um comportamento estranho da turbina: ADC bruto da
// bateria, tensao calculada, tensao pedida e o PWM que realmente foi
// escrito no pino. So existe por causa do teste de bancada, 03/08/2026,
// onde a turbina ligou/desligou varias vezes rapido no meio de um TN -
// sem esses numeros no momento exato, nao da pra saber se foi a leitura
// de bateria oscilando, um comando repetido chegando, ou outra coisa.
static void send_fan_telemetry(const char *phase) {
    String v_pedida = (last_requested_voltage < 0) ? "manual" : String(last_requested_voltage, 2);

    String line = "FAN t=" + String(millis())
                + " fase=" + phase
                + " adcBat=" + String(get_battery_raw_adc())
                + " vBat=" + String(get_battery_voltage(), 2)
                + " vPedida=" + v_pedida
                + " pwm=" + String(last_pwm_to_fan);

    Serial.println("[validar_fan] " + line);
    send_bluetooth_message(line);
}

// Espera 'ms', mandando telemetria (ver send_fan_telemetry()) a cada
// ~150ms, mas sai mais cedo se chegar QUALQUER comando por Bluetooth ou
// Serial nesse meio tempo - pra dar um jeito de abortar o teste sem
// esperar o delay fixo terminar. Devolve true se foi interrompido.
static bool interruptible_delay_with_telemetry(unsigned long ms, const char *phase) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        send_fan_telemetry(phase);

        if (read_bluetooth_message() != "") return true;
        if (Serial.available()) {
            Serial.readStringUntil('\n');
            return true;
        }
        delay(150);
    }
    return false;
}

// PWM manual do teste de bancada (0-255) - ajustar aqui direto, sem
// depender da calibracao de bateria (ver set_fan_pwm_manual()).
#define VALIDAR_FAN_PWM_MANUAL 70

// Validacao de bancada: liga a turbina em potencia baixa por 5s, depois
// desliga e para (sem fase OFF de espera - so confirma que desligou).
// Testar com a turbina longe de fios soltos/dedos/cabelo. Envie QUALQUER
// comando (Bluetooth ou Serial) a qualquer momento pra abortar - desliga
// a turbina na hora e cancela o resto do teste. Manda telemetria completa
// por Bluetooth o tempo todo (ver send_fan_telemetry()).
void validar_fan() {
    Serial.printf("[validar_fan] ligando turbina (PWM manual %d/255, envie qualquer comando pra abortar)\n", VALIDAR_FAN_PWM_MANUAL);
    set_fan_pwm_manual(VALIDAR_FAN_PWM_MANUAL);
    if (interruptible_delay_with_telemetry(5000, "ON")) {
        set_fan_pwm_manual(0);
        Serial.println("[validar_fan] ABORTADO - comando recebido, turbina desligada.");
        return;
    }

    Serial.println("[validar_fan] desligando turbina");
    set_fan_pwm_manual(0);

    Serial.println("[validar_fan] fim");
}
