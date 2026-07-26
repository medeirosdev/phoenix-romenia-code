#ifndef __LED_H__
#define __LED_H__

#include "utils.h"

// Precisa ser definido ANTES do #include <FastLED.h> (em led.cpp, que
// inclui este header primeiro). Ajuda o FastLED a se recuperar quando o
// timing do sinal e interrompido por outro periferico interrupt-heavy -
// nesse robo, o Bluetooth. Sem isso, os LEDs podem piscar/corromper cor
// quando o BLE estiver ativo (mesmo ajuste que o projeto antigo usava).
#define FASTLED_INTERRUPT_RETRY_COUNT 1

#define DELAY_TEST 300 // ms entre cores na validacao

// Cores (RGB) - mesma paleta do projeto antigo.
#define RED     255, 0, 0
#define ORANGE  255, 48, 0
#define YELLOW  255, 115, 0
#define GREEN   0, 230, 0
#define CIAN    0, 255, 255
#define BLUE    0, 0, 255
#define PURPLE  137, 0, 255
#define PINK    240, 0, 80
#define WHITE   200, 200, 200
#define LED_OFF 0, 0, 0

// Os 2 grupos de LED sao INDEPENDENTES (pinos/tiras diferentes, nao estao
// em serie um com o outro - ver PLANEJAMENTO.md secao 4.2).
enum Led_Group {
    LED_PRINCIPAL, // 3 LEDs
    LED_FRONTAL    // 2 LEDs
};

void led_init();
void set_led_color(Led_Group group, uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void set_group_color(Led_Group group, uint8_t red, uint8_t green, uint8_t blue);
void set_all_leds_color(uint8_t red, uint8_t green, uint8_t blue);
void clear_leds();
void validar_led();

#endif /* __LED_H__ */
