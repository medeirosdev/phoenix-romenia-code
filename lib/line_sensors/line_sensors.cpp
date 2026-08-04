#include "line_sensors.h"
#include "config.h"
#include "AD7490.h"
#include "bluetooth.h"

static FrontalSensors FS;
static bool last_on_line = false;

static void reset_sensors_calibration() {
    for (uint8_t sensor = 0; sensor < NUMBER_OF_FRONTAL_SENSORS; sensor++) {
        FS.max_calibration_reading[sensor] = 0;
        FS.min_calibration_reading[sensor] = MAX_ADC_READING;
    }
}

static void read_frontal_sensors_adc() {
    for (uint8_t sensor = 0; sensor < NUMBER_OF_FRONTAL_SENSORS; sensor++) {
        uint32_t sum_of_readings = 0;
        for (uint8_t i = 0; i < NUMBER_OF_SAMPLES_PER_READING; i++) {
            sum_of_readings += read_AD7490_channel(sensor);
        }
        FS.adc_reading[sensor] = sum_of_readings / NUMBER_OF_SAMPLES_PER_READING;
    }
}

static void compute_calibration_extremes() {
    for (uint8_t sensor = 0; sensor < NUMBER_OF_FRONTAL_SENSORS; sensor++) {
        if (FS.adc_reading[sensor] > FS.max_calibration_reading[sensor]) {
            FS.max_calibration_reading[sensor] = FS.adc_reading[sensor];
        }
        if (FS.adc_reading[sensor] < FS.min_calibration_reading[sensor]) {
            FS.min_calibration_reading[sensor] = FS.adc_reading[sensor];
        }
    }
}

// Normaliza as leituras pra escala 0..MAX_NORMALIZED_VALUE usando a
// calibracao de cada sensor, e aplica o MODO_ROMENIA.
//
// O sensor sempre devolve leitura ALTA em cima do preto e BAIXA em cima
// do branco (isso e a fisica do sensor, nao muda). O que muda e qual das
// duas cores e "a linha" que queremos seguir - por isso, apos normalizar,
// so invertemos a escala se a linha for BRANCA (MODO_ROMENIA=false), pra
// "valor alto" sempre significar "em cima da linha", nao importa a cor.
static void normalize_frontal_sensors_readings() {
    read_frontal_sensors_adc();

    for (uint8_t sensor = 0; sensor < NUMBER_OF_FRONTAL_SENSORS; sensor++) {
        uint16_t sensor_min = FS.min_calibration_reading[sensor];
        uint16_t sensor_max = FS.max_calibration_reading[sensor];

        // Sensor sem faixa de calibracao valida - ou nunca variou durante
        // a calibracao (provavel defeito/desconectado), ou a calibracao
        // nunca rodou (fica com o default max=0 < min=4095). map() faria
        // divisao por zero no primeiro caso, e devolveria lixo invertido
        // no segundo - em vez disso, trata como "sem leitura" (valor
        // baixo, abaixo do noise_threshold), sem contaminar a media
        // ponderada em read_robot_position(). Achado em revisao de
        // codigo, 22/07/2026.
        if (sensor_max <= sensor_min) {
            FS.normalized_reading[sensor] = 0;
            continue;
        }

        uint16_t reading = constrain(FS.adc_reading[sensor], sensor_min, sensor_max);

        FS.normalized_reading[sensor] = map(reading, sensor_min, sensor_max, 0, MAX_NORMALIZED_VALUE);

        if (!MODO_ROMENIA) {
            // Linha branca (testes de bancada): inverte, pra linha continuar
            // sendo o valor alto.
            FS.normalized_reading[sensor] = MAX_NORMALIZED_VALUE - FS.normalized_reading[sensor];
        }
    }
}

void reset_line_sensors_calibration() {
    reset_sensors_calibration();
}

void set_line_threshold(float given_threshold) {
    FS.line_threshold = given_threshold;
}

void set_noise_threshold(float given_threshold) {
    FS.noise_threshold = given_threshold;
}

// Calcula a posicao do robo em relacao a linha por media ponderada das
// leituras normalizadas (sensores mais "acesos" pesam mais). Se nenhum
// sensor detectar a linha, mantem a ultima posicao conhecida no extremo
// (esquerda ou direita) - assume que o robo saiu da pista por aquele lado.
float read_robot_position() {
    bool on_line = false;
    uint32_t weighted_readings = 0;
    uint32_t sum_of_readings = 0;

    normalize_frontal_sensors_readings();

    for (uint8_t sensor = 0; sensor < NUMBER_OF_FRONTAL_SENSORS; sensor++) {
        if (FS.normalized_reading[sensor] > FS.line_threshold) on_line = true;

        if (FS.normalized_reading[sensor] > FS.noise_threshold) {
            weighted_readings += (uint32_t)FS.normalized_reading[sensor] * sensor;
            sum_of_readings += FS.normalized_reading[sensor];
        }
    }

    last_on_line = on_line;

    if (!on_line) {
        if (FS.position < 0) FS.position = -(NUMBER_OF_FRONTAL_SENSORS - 1) * 0.5f;
        else FS.position = (NUMBER_OF_FRONTAL_SENSORS - 1) * 0.5f;
        return FS.position;
    }

    FS.position = (float)weighted_readings / sum_of_readings;
    FS.position -= (NUMBER_OF_FRONTAL_SENSORS - 1) * 0.5f;

    return FS.position;
}

bool line_sensors_is_on_line() {
    return last_on_line;
}

// Calibracao de bancada: passa a barra de sensores sobre a linha e o
// fundo repetidas vezes durante essa janela de tempo, guardando o maior
// e o menor valor lido por sensor.
void calibrate_line_sensors() {
    reset_sensors_calibration();

    Serial.println("[line_sensors] Calibracao iniciada - passe a barra sobre a linha e o fundo");
    unsigned long calibration_start_time = millis();
    while (millis() - calibration_start_time <= CALIBRATION_DURATION_MS) {
        read_frontal_sensors_adc();
        compute_calibration_extremes();
    }
    Serial.println("[line_sensors] Calibracao terminada");
}

void set_line_sensors_calibration_values(
    short max_values[NUMBER_OF_FRONTAL_SENSORS],
    short min_values[NUMBER_OF_FRONTAL_SENSORS]
) {
    for (uint8_t sensor = 0; sensor < NUMBER_OF_FRONTAL_SENSORS; sensor++) {
        FS.max_calibration_reading[sensor] = max_values[sensor];
        FS.min_calibration_reading[sensor] = min_values[sensor];
    }
}

void get_line_sensors_calibration_values(
    short target_max_values[NUMBER_OF_FRONTAL_SENSORS],
    short target_min_values[NUMBER_OF_FRONTAL_SENSORS]
) {
    for (uint8_t sensor = 0; sensor < NUMBER_OF_FRONTAL_SENSORS; sensor++) {
        target_max_values[sensor] = FS.max_calibration_reading[sensor];
        target_min_values[sensor] = FS.min_calibration_reading[sensor];
    }
}

void line_sensors_init() {
    AD7490_init();
    reset_sensors_calibration();
    set_line_threshold(ON_LINE_THRESHOLD);
    set_noise_threshold(NOISE_THRESHOLD);
}

// Validacao de bancada: imprime a leitura CRUA (antes de calibrar) dos
// NUMBER_OF_FRONTAL_SENSORS sensores frontais por 10s. Serve pra conferir a fiacao/SPI antes de
// rodar uma calibracao de verdade - nao depende de calibracao nenhuma.
// Manda a mesma leitura por Bluetooth (CSV: "1234,1235,...") pra quando o
// comando TF vier do app, e nao so do Serial Monitor - sem isso o app
// nao recebia nada de volta.
void validar_sensores_frontais() {
    unsigned long start_time = millis();

    while (millis() - start_time < 10000) {
        read_frontal_sensors_adc();

        Serial.print("[validar_sensores_frontais] ");
        String csv = "";
        for (uint8_t sensor = 0; sensor < NUMBER_OF_FRONTAL_SENSORS; sensor++) {
            Serial.printf("S%d:%4d ", sensor, FS.adc_reading[sensor]);
            if (sensor > 0) csv += ",";
            csv += FS.adc_reading[sensor];
        }
        Serial.println();
        send_bluetooth_message(csv);

        delay(200);
    }
}
