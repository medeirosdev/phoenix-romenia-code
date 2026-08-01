#ifndef __PINOUT_H__
#define __PINOUT_H__

// =====================================================================
// PINOUT DO ROBO ROMENIA (ESP32-S3-WROOM-1-N16R8)
//
// Fonte: "Pinos da Bia Romênia.pdf" (documento de referencia dedicado,
// ver PLANEJAMENTO.md secao 4.5). Esse arquivo e o UNICO lugar que deve
// ser editado quando um pino for corrigido.
// =====================================================================

// BOOT / STRAPPING (nao usar como GPIO de aplicacao)
#define BOOT_ESP_PIN            0   // GPIO0: pino de boot do ESP32-S3, nao mexer
// GPIO3 esta desconectado (confirmado por Ricardo via WhatsApp, 20/07/2026)

// BATERIA
#define BATTERY_LEVEL_PIN       2   // leitura analogica da tensao da bateria

// INFRAVERMELHO
#define IR_RECEIVER_PIN         4   // recebe o sinal do receptor de infravermelho

// LEDS (FastLED) - dois grupos INDEPENDENTES, cada um com seu proprio pino
// de dados (nao estao em serie, ver PLANEJAMENTO.md secao 4.2).
#define LED_FRONTAL_PIN          5   // 2 LEDs, placa frontal
#define LED_PRINCIPAL_PIN       17   // 3 LEDs, placa principal
#define NUMBER_OF_LEDS_PRINCIPAL 3
#define NUMBER_OF_LEDS_FRONTAL   2

// SENSORES FRONTAIS DE LINHA (AD7490 via SPI)
#define FRONTAL_SENSORS_CS_PIN   6   // Chip Select
#define FRONTAL_SENSORS_DIN_PIN  7   // data in (ESP32 -> AD7490)
#define FRONTAL_SENSORS_SCLK_PIN 15  // Clock ("dita o ritmo")
#define FRONTAL_SENSORS_DOUT_PIN 16  // data out (AD7490 -> ESP32)

// NAO EXISTEM SENSORES LATERAIS neste robo - o header "SOS" e so reserva
// de pinos pra caso um pad da placa quebre ("caso de algum bo"),
// confirmado pelo usuario. Nao criar defines pra eles, nao ha nada pra
// ler ali (ver PLANEJAMENTO.md secao 4.4).

// TURBINA DE SUCCAO (fan)
// Pino unico de PWM (turbina nao inverte sentido): 0 = parada, 255 =
// tensao maxima da bateria - confirmado no documento de referencia.
#define FAN_MOTOR_PIN            9

// MOTORES DE TRACAO
// Esquerda/direita CONFIRMADOS no documento de referencia (antes disso
// so seria possivel confirmar testando na bancada). A ordem de qual pino
// de cada par e IN1/IN2 (terminal A/B) NAO foi especificada no documento -
// se o motor girar ao contrario do esperado, e so inverter o par abaixo.
#define LEFT_MOTOR_IN1_PIN       14
#define LEFT_MOTOR_IN2_PIN       21
#define RIGHT_MOTOR_IN1_PIN      10
#define RIGHT_MOTOR_IN2_PIN      11

// USB (nativo do ESP32-S3) - reservados, nao usar como GPIO
#define USB_DM_PIN               19  // diferencial negativo
#define USB_DP_PIN               20  // diferencial positivo

#endif
