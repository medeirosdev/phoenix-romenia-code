#ifndef __EEPROM_MANAGER_H__
#define __EEPROM_MANAGER_H__

#include "utils.h"

void eeprom_manager_init();
void save_line_sensors_calibration();

// Retorna false se nao tinha nenhuma calibracao valida salva (EEPROM
// "virgem" ou apagada por erase_line_sensors_calibration()) - nesse caso
// os valores de line_sensors NAO sao alterados.
bool load_line_sensors_calibration();

// "Reset de fabrica" (PLANEJAMENTO.md secao 9): invalida a calibracao
// salva, pra nao correr risco de competir com dado de uma sessao antiga
// sem perceber.
void erase_line_sensors_calibration();

void validar_eeprom();

#endif /* __EEPROM_MANAGER_H__ */
