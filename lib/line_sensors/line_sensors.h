#ifndef __LINE_SENSORS_H__
#define __LINE_SENSORS_H__

#include "utils.h"

// Nao existem sensores laterais neste robo (confirmado - o header "SOS"
// e so reserva de pinos, ver PLANEJAMENTO.md secao 4.4). Essa lib cuida
// SO dos sensores frontais.
#define NUMBER_OF_FRONTAL_SENSORS      9
#define NUMBER_OF_SAMPLES_PER_READING  2   // leituras por amostra (reduz ruido)
#define CALIBRATION_DURATION_MS        5000

#define MAX_NORMALIZED_VALUE 4000
#define MAX_ADC_READING       4095 // 12 bits (AD7490)

// Fracoes do MAX_NORMALIZED_VALUE - mesmos valores usados no projeto
// antigo, ponto de partida razoavel, mas podem precisar de ajuste fino
// depois de calibrar na pista real.
#define ON_LINE_THRESHOLD ((uint16_t)(0.2   * MAX_NORMALIZED_VALUE))
#define NOISE_THRESHOLD   ((uint16_t)(0.065 * MAX_NORMALIZED_VALUE))

typedef struct {
    uint16_t adc_reading[NUMBER_OF_FRONTAL_SENSORS];
    uint16_t max_calibration_reading[NUMBER_OF_FRONTAL_SENSORS];
    uint16_t min_calibration_reading[NUMBER_OF_FRONTAL_SENSORS];
    uint16_t normalized_reading[NUMBER_OF_FRONTAL_SENSORS];
    float position;
    uint16_t line_threshold;
    uint16_t noise_threshold;
} FrontalSensors;

void line_sensors_init();
void calibrate_line_sensors();
void set_line_sensors_calibration_values(
    short max_values[NUMBER_OF_FRONTAL_SENSORS],
    short min_values[NUMBER_OF_FRONTAL_SENSORS]
);
void get_line_sensors_calibration_values(
    short target_max_values[NUMBER_OF_FRONTAL_SENSORS],
    short target_min_values[NUMBER_OF_FRONTAL_SENSORS]
);
void set_line_threshold(float given_threshold);
void set_noise_threshold(float given_threshold);

// Retorna a posicao do robo em relacao a linha: 0 = centralizado,
// negativo = linha pra esquerda, positivo = linha pra direita. Faixa
// aproximada: -(N-1)/2 a +(N-1)/2, onde N = NUMBER_OF_FRONTAL_SENSORS.
float read_robot_position();

// Diz se, na ULTIMA chamada de read_robot_position(), pelo menos 1 sensor
// detectou a linha de verdade (acima do line_threshold) - nao chama uma
// leitura nova, so reporta o resultado da ultima. Usado pelo failsafe de
// "saiu da linha" (ver PLANEJAMENTO.md secao 12 e config.h).
bool line_sensors_is_on_line();

void validar_sensores_frontais();

#endif /* __LINE_SENSORS_H__ */
