#ifndef __PINOUT_H__
#define __PINOUT_H__

// =====================================================================
// PINOUT DO ROBO ROMENIA (ESP32-S3-WROOM-1-N16R8)
//
// Fonte: codigo_romenia/image.png (schematic) + confirmacoes do Ricardo
// (ver codigo_romenia/PLANEJAMENTO.md, secao 4, para o historico completo).
//
// Pinos marcados "TODO" ainda NAO foram confirmados no schematic real -
// a leitura da imagem ja teve pelo menos 1 erro (secao 4.5 do
// PLANEJAMENTO.md), entao trate qualquer valor aqui como provisorio ate
// ser confirmado. Esse arquivo e o UNICO lugar que deve ser editado
// quando um pino for corrigido.
// =====================================================================

// BOOT / STRAPPING (nao usar como GPIO de aplicacao)
#define BOOT_ESP_PIN            0   // GPIO0: pino de boot do ESP32-S3, nao mexer

// BATERIA
#define BATTERY_LEVEL_PIN       1   // leitura analogica da tensao da bateria

// INFRAVERMELHO
#define IR_RECEIVER_PIN         4   // confirmado por Ricardo (20/07/2026)
// GPIO3 esta desconectado no schematic (X) - nao usar pra nada

// LEDS (FastLED) - dois grupos INDEPENDENTES, cada um com seu proprio pino
// de dados (nao estao em serie). Nenhum dos dois pinos esta confirmado
// ainda - ver PLANEJAMENTO.md secao 4.2/4.5. Valor 99 = pino invalido de
// proposito (nenhum GPIO real do ESP32-S3 chega perto disso), pra falhar
// alto se alguem tentar usar antes de confirmar o pino de verdade -
// NAO e um numero de GPIO, e um sinalizador de "ainda nao sei".
#define LED_PRINCIPAL_PIN       99  // TODO: descobrir o pino real (3 LEDs, grupo "principal")
#define LED_FRONTAL_PIN         99  // TODO: descobrir o pino real (2 LEDs, grupo "frontal")
#define NUMBER_OF_LEDS_PRINCIPAL 3
#define NUMBER_OF_LEDS_FRONTAL   2

// SENSORES FRONTAIS DE LINHA (AD7490 via SPI)
#define FRONTAL_SENSORS_CS_PIN   5   // Chip Select
#define FRONTAL_SENSORS_DIN_PIN  6   // MOSI (ESP32 -> AD7490)
#define FRONTAL_SENSORS_SCLK_PIN 14  // Clock
#define FRONTAL_SENSORS_DOUT_PIN 15  // MISO (AD7490 -> ESP32)

// NAO EXISTEM SENSORES LATERAIS neste robo - o header "SOS" (IO35-IO40) e
// so reserva de pinos pra caso um pad da placa quebre, confirmado pelo
// usuario. Nao criar defines pra eles, nao ha nada pra ler ali.

// TURBINA DE SUCCAO (fan) - confirmado por Ricardo: MT ESP = motor da turbina
#define FAN_MOTOR_PIN            8   // pino unico de PWM (turbina nao inverte sentido)

// MOTORES DE TRACAO
// Confirmado por Ricardo: PWM A+B = Motor 1, PWM C+D = Motor 2.
// Falta confirmar qual motor (1 ou 2) fica fisicamente a esquerda/direita
// no robo montado - ajustar os nomes abaixo quando isso for confirmado.
#define MOTOR_1_IN1_PIN          13  // PWM A
#define MOTOR_1_IN2_PIN          21  // PWM B
#define MOTOR_2_IN1_PIN          9   // PWM C
#define MOTOR_2_IN2_PIN          10  // PWM D

#endif
