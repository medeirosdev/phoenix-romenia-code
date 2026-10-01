#ifndef __AD_7490_H__
#define __AD_7490_H__

#include "utils.h"

// SPI
#define SPI_FREQUENCY  2e7  // 20 MHz - maximo do AD7490; se o TF ficar instavel, voltar pra 1e7
#define SPI_BIT_ORDER  MSBFIRST
#define SPI_MODE       SPI_MODE0

// Registrador de controle (CR) do AD7490 - ver datasheet do chip pra
// entender cada campo. Valores herdados do projeto antigo (bia-senna-code-2026),
// nao mudam entre placas - sao do protocolo do proprio chip, nao do robo.
#define AD7490_CR_WRITE_VALUE  1
#define AD7490_CR_SEQ_VALUE    0
#define AD7490_CR_PM_VALUE     3
#define AD7490_CR_SHADOW_VALUE 0
#define AD7490_CR_WEAK_VALUE   1
#define AD7490_CR_RANGE_VALUE  1
#define AD7490_CR_CODING_VALUE 1

void AD7490_init();
uint16_t read_AD7490_channel(uint8_t channel);

// Le os canais 0..count-1 em sequencia (pipeline, ver AD7490.cpp) e grava
// em out[]. Bem mais rapido que chamar read_AD7490_channel() por canal.
void read_AD7490_all(uint16_t *out, uint8_t count);

// Quantas respostas vieram com o numero de canal (4 bits de cima) diferente
// do esperado desde o ultimo reset - deve ficar em 0. Diferente de 0 indica
// leitura fora de ordem ou SPI instavel (ex.: 20 MHz demais pra placa).
uint32_t get_AD7490_channel_errors();
void reset_AD7490_channel_errors();

void validar_AD7490();

#endif
