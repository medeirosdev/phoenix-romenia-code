#ifndef __AD_7490_H__
#define __AD_7490_H__

#include "utils.h"

// SPI
#define SPI_FREQUENCY  1e7  // 10 MHz
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
void validar_AD7490();

#endif
