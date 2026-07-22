#include "AD7490.h"
#include "pinout.h"
#include <SPI.h>

// Monta o comando SPI que pede a leitura de um canal especifico do
// AD7490. Ver datasheet do chip pra detalhes de cada campo do registrador.
static uint16_t generate_AD7490_command(uint8_t addr) {
    return ((
        (AD7490_CR_WRITE_VALUE  << 11) |
        (AD7490_CR_SEQ_VALUE    << 10) |
        ((uint16_t)addr         << 6)  |
        (AD7490_CR_PM_VALUE     << 4)  |
        (AD7490_CR_SHADOW_VALUE << 3)  |
        (AD7490_CR_WEAK_VALUE   << 2)  |
        (AD7490_CR_RANGE_VALUE  << 1)  |
        (AD7490_CR_CODING_VALUE)
    ) << 4);
}

static uint16_t write_to_AD7490(uint16_t command) {
    digitalWrite(FRONTAL_SENSORS_CS_PIN, LOW);
    SPI.beginTransaction(SPISettings(SPI_FREQUENCY, SPI_BIT_ORDER, SPI_MODE));
    uint16_t response = SPI.transfer16(command);
    SPI.endTransaction();
    digitalWrite(FRONTAL_SENSORS_CS_PIN, HIGH);

    return response;
}

// Le um canal do AD7490. Por causa de como o SPI desse chip funciona, o
// dado pedido so chega na proxima transacao - por isso manda o comando,
// depois manda um comando "em branco" so pra puxar a resposta.
//
// A troca "channel = 3 - channel" pros 4 primeiros canais e herdada do
// projeto antigo (bia-senna-code-2026) - provavelmente compensa a ordem
// fisica de fiacao dos primeiros 4 sensores na barra. Como a barra de
// sensores frontais e a mesma do robo antigo, mantive igual, mas isso
// PRECISA ser conferido na bancada com o hardware novo (comparar leitura
// de cada canal com qual sensor fisico esta sendo tocado).
uint16_t read_AD7490_channel(uint8_t channel) {
    if (channel < 4) channel = 3 - channel;
    uint16_t command = generate_AD7490_command(channel);

    write_to_AD7490(command);
    uint16_t response = write_to_AD7490(0);
    response = response & 0x0FFF;
    return response;
}

static void reset_AD7490() {
    // Ver datasheet do AD7490, pagina 21.
    write_to_AD7490(UINT16_MAX);
    delay(10);
    write_to_AD7490(UINT16_MAX);
    delay(10);

    uint16_t command = generate_AD7490_command(0);
    write_to_AD7490(command);
}

void AD7490_init() {
    pinMode(FRONTAL_SENSORS_CS_PIN, OUTPUT);
    digitalWrite(FRONTAL_SENSORS_CS_PIN, HIGH);

    SPI.begin(
        FRONTAL_SENSORS_SCLK_PIN,
        FRONTAL_SENSORS_DOUT_PIN,
        FRONTAL_SENSORS_DIN_PIN,
        FRONTAL_SENSORS_CS_PIN
    );

    reset_AD7490();
}

// Validacao de bancada: le e imprime os primeiros 15 canais do AD7490
// (o robo usa 9, mas ler mais ajuda a confirmar que o chip inteiro
// responde). Serve pra checar a fiacao do SPI antes de confiar na leitura.
void validar_AD7490() {
    for (uint8_t channel = 0; channel < 15; channel++) {
        uint16_t reading = read_AD7490_channel(channel);
        Serial.printf("[validar_AD7490] canal %d: %d\n", channel, reading);
    }
    Serial.println();
    delay(2000);
}
