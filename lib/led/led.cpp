#include "led.h"
#include "pinout.h"
#include <FastLED.h>

// Duas tiras FastLED INDEPENDENTES, uma por grupo - nao e um unico array
// de 5 pixels encadeados, sao 2 pinos de dados separados (ver pinout.h).
static CRGB principal_leds[NUMBER_OF_LEDS_PRINCIPAL];
static CRGB frontal_leds[NUMBER_OF_LEDS_FRONTAL];

void led_init() {
    FastLED.addLeds<NEOPIXEL, LED_PRINCIPAL_PIN>(principal_leds, NUMBER_OF_LEDS_PRINCIPAL);
    FastLED.addLeds<NEOPIXEL, LED_FRONTAL_PIN>(frontal_leds, NUMBER_OF_LEDS_FRONTAL);
    clear_leds();
}

void set_led_color(Led_Group group, uint8_t index, uint8_t red, uint8_t green, uint8_t blue) {
    if (group == LED_PRINCIPAL) {
        if (index >= NUMBER_OF_LEDS_PRINCIPAL) return;
        principal_leds[index] = CRGB(red, green, blue);
    } else {
        if (index >= NUMBER_OF_LEDS_FRONTAL) return;
        frontal_leds[index] = CRGB(red, green, blue);
    }
    FastLED.show();
}

void set_group_color(Led_Group group, uint8_t red, uint8_t green, uint8_t blue) {
    if (group == LED_PRINCIPAL) {
        fill_solid(principal_leds, NUMBER_OF_LEDS_PRINCIPAL, CRGB(red, green, blue));
    } else {
        fill_solid(frontal_leds, NUMBER_OF_LEDS_FRONTAL, CRGB(red, green, blue));
    }
    FastLED.show();
}

void set_all_leds_color(uint8_t red, uint8_t green, uint8_t blue) {
    fill_solid(principal_leds, NUMBER_OF_LEDS_PRINCIPAL, CRGB(red, green, blue));
    fill_solid(frontal_leds, NUMBER_OF_LEDS_FRONTAL, CRGB(red, green, blue));
    FastLED.show();
}

void clear_leds() {
    set_all_leds_color(LED_OFF);
}

// Validacao de bancada: passa por uma sequencia de cores, primeiro no
// grupo principal (3 LEDs), depois no frontal (2 LEDs), pra confirmar
// que cada tira responde no pino certo.
void validar_led() {
    struct { const char *name; uint8_t r, g, b; } colors[] = {
        {"RED", RED}, {"ORANGE", ORANGE}, {"YELLOW", YELLOW}, {"GREEN", GREEN},
        {"CIAN", CIAN}, {"BLUE", BLUE}, {"PURPLE", PURPLE}, {"PINK", PINK}, {"WHITE", WHITE}
    };
    const uint8_t number_of_colors = sizeof(colors) / sizeof(colors[0]);

    Serial.println("[validar_led] Grupo PRINCIPAL (3 LEDs, pino LED_PRINCIPAL_PIN)");
    for (uint8_t i = 0; i < number_of_colors; i++) {
        Serial.printf("[validar_led]   %s\n", colors[i].name);
        set_group_color(LED_PRINCIPAL, colors[i].r, colors[i].g, colors[i].b);
        delay(DELAY_TEST);
    }
    clear_leds();

    Serial.println("[validar_led] Grupo FRONTAL (2 LEDs, pino LED_FRONTAL_PIN)");
    for (uint8_t i = 0; i < number_of_colors; i++) {
        Serial.printf("[validar_led]   %s\n", colors[i].name);
        set_group_color(LED_FRONTAL, colors[i].r, colors[i].g, colors[i].b);
        delay(DELAY_TEST);
    }
    clear_leds();

    Serial.println("[validar_led] fim");
}
