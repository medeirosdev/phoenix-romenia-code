#include "battery.h"
#include "pinout.h"

// Valor inicial otimista (bateria "alta" pro numero de celulas configurado),
// so ate a primeira leitura real acontecer.
float voltage = BATTERY_CELL_COUNT * VOLTS_PER_CELL_HIGH;
Battery_Status battery_status = BATTERY_HIGH;

unsigned long battery_low_since = 0; // 0 = nao esta em bateria fraca no momento

void battery_init() {
    pinMode(BATTERY_LEVEL_PIN, INPUT);
}

// Media aritmetica de N leituras do ADC, pra reduzir ruido na medicao.
static uint16_t read_battery_adc(uint8_t number_of_readings) {
    uint32_t sum_of_readings = 0;
    for (uint8_t i = 0; i < number_of_readings; i++) {
        sum_of_readings += analogRead(BATTERY_LEVEL_PIN);
    }
    return sum_of_readings / number_of_readings;
}

// Converte a leitura crua do ADC em tensao real, usando os parametros de
// calibracao (proporcao linear - ver comentario no battery.h).
static void read_battery_voltage() {
    uint16_t adc_reading = read_battery_adc(BATTERY_SAMPLE_READINGS);
    voltage = ((float)adc_reading * BATTERY_VOLTAGE_PARAMETER) / BATTERY_ADC_PARAMETER;
}

static void read_battery_status() {
    if (voltage >= BATTERY_HIGH_THRESHOLD) {
        battery_status = BATTERY_HIGH;
    } else if (voltage > BATTERY_LOW_THRESHOLD) {
        battery_status = BATTERY_MEDIUM;
    } else if (voltage > USB_THRESHOLD) {
        battery_status = BATTERY_LOW;
    } else {
        battery_status = USB_POWER;
    }
}

// So contabiliza ha quanto tempo a bateria esta fraca sem interrupcao.
// NAO desliga nada sozinha - o que fazer com o failsafe (parar o robo?
// so avisar?) ainda e uma decisao em aberto, ver PLANEJAMENTO.md secao 12.
static void update_battery_failsafe_timer() {
    if (battery_status == BATTERY_LOW) {
        if (battery_low_since == 0) battery_low_since = millis();
    } else {
        battery_low_since = 0;
    }
}

void battery_monitoring() {
    read_battery_voltage();
    read_battery_status();
    update_battery_failsafe_timer();
}

Battery_Status get_battery_status() {
    return battery_status;
}

float get_battery_voltage() {
    return voltage;
}

bool battery_failsafe_triggered() {
    return battery_low_since != 0 && (millis() - battery_low_since) >= FAILSAFE_TIMEOUT;
}

// Validacao de bancada: imprime tensao e status por 10s. Serve pra conferir
// se o BATTERY_LEVEL_PIN esta ligado certo e se os parametros de calibracao
// ainda fazem sentido pra bateria em uso (comparar com um multimetro).
void validar_bateria() {
    const char *status_names[] = {"ALTA", "MEDIA", "BAIXA", "USB"};
    unsigned long start_time = millis();

    while (millis() - start_time < 10000) {
        battery_monitoring();
        Serial.printf(
            "[validar_bateria] %.2f V | status: %s\n",
            get_battery_voltage(),
            status_names[get_battery_status()]
        );
        delay(500);
    }
}
