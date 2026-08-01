#include "eeprom_manager.h"
#include "line_sensors.h"
#include <EEPROM.h>

// So grava a calibracao dos sensores frontais - sem sensores laterais
// pra salvar, e sem o esquema de "2 estados" (TEST/COMPETITION) do
// projeto antigo, simplificado pra 1 bloco so (PLANEJAMENTO.md secao 9).
#define EEPROM_SIZE 64 // bem mais que o necessario (~38 bytes), margem de sobra

// "Assinatura" gravada junto com os dados - se nao bater na leitura,
// consideramos que a EEPROM esta "virgem" (nunca foi salva calibracao
// nenhuma, ou foi apagada de proposito).
#define EEPROM_MAGIC_NUMBER  ((uint16_t)0xB1A0)
#define EEPROM_MAGIC_ADDRESS 0
#define EEPROM_MAX_ADDRESS   (EEPROM_MAGIC_ADDRESS + sizeof(uint16_t))
#define EEPROM_MIN_ADDRESS   (EEPROM_MAX_ADDRESS + sizeof(short) * NUMBER_OF_FRONTAL_SENSORS)

void eeprom_manager_init() {
    EEPROM.begin(EEPROM_SIZE);
}

void save_line_sensors_calibration() {
    short max_values[NUMBER_OF_FRONTAL_SENSORS];
    short min_values[NUMBER_OF_FRONTAL_SENSORS];
    get_line_sensors_calibration_values(max_values, min_values);

    EEPROM.put(EEPROM_MAX_ADDRESS, max_values);
    EEPROM.put(EEPROM_MIN_ADDRESS, min_values);
    EEPROM.put(EEPROM_MAGIC_ADDRESS, EEPROM_MAGIC_NUMBER);
    EEPROM.commit();

    Serial.println("[eeprom_manager] Calibracao salva.");
}

bool load_line_sensors_calibration() {
    uint16_t magic;
    EEPROM.get(EEPROM_MAGIC_ADDRESS, magic);

    if (magic != EEPROM_MAGIC_NUMBER) {
        Serial.println("[eeprom_manager] Nenhuma calibracao salva (EEPROM virgem).");
        return false;
    }

    short max_values[NUMBER_OF_FRONTAL_SENSORS];
    short min_values[NUMBER_OF_FRONTAL_SENSORS];
    EEPROM.get(EEPROM_MAX_ADDRESS, max_values);
    EEPROM.get(EEPROM_MIN_ADDRESS, min_values);
    set_line_sensors_calibration_values(max_values, min_values);

    Serial.println("[eeprom_manager] Calibracao carregada da EEPROM.");
    return true;
}

// Invalida a assinatura na EEPROM (load_line_sensors_calibration() passa
// a tratar como "nao tem nada salvo") E reseta a calibracao que ja esta
// carregada em RAM - sem o reset em RAM, o robo continuava correndo com
// a calibracao antiga ate a placa reiniciar, mesmo depois do RF (achado
// em revisao de codigo, 31/07/2026).
void erase_line_sensors_calibration() {
    EEPROM.put(EEPROM_MAGIC_ADDRESS, (uint16_t)0x0000);
    EEPROM.commit();
    reset_line_sensors_calibration();
    Serial.println("[eeprom_manager] Calibracao apagada (EEPROM e memoria).");
}

// Validacao de bancada: salva a calibracao atual, zera a copia em
// memoria (pra garantir que nao sobrou nada), recarrega da EEPROM e
// confere se bateu com o que foi salvo. Testa o ciclo completo sem
// precisar desligar/religar o robo de verdade.
void validar_eeprom() {
    Serial.println("[validar_eeprom] Salvando calibracao atual...");
    save_line_sensors_calibration();

    short saved_max[NUMBER_OF_FRONTAL_SENSORS];
    short saved_min[NUMBER_OF_FRONTAL_SENSORS];
    get_line_sensors_calibration_values(saved_max, saved_min);

    short zeros[NUMBER_OF_FRONTAL_SENSORS] = {0};
    set_line_sensors_calibration_values(zeros, zeros);

    Serial.println("[validar_eeprom] Recarregando da EEPROM...");
    if (!load_line_sensors_calibration()) {
        Serial.println("[validar_eeprom] FALHOU: nao achou calibracao salva.");
        return;
    }

    short loaded_max[NUMBER_OF_FRONTAL_SENSORS];
    short loaded_min[NUMBER_OF_FRONTAL_SENSORS];
    get_line_sensors_calibration_values(loaded_max, loaded_min);

    bool ok = true;
    for (uint8_t sensor = 0; sensor < NUMBER_OF_FRONTAL_SENSORS; sensor++) {
        if (loaded_max[sensor] != saved_max[sensor] || loaded_min[sensor] != saved_min[sensor]) {
            ok = false;
            Serial.printf(
                "[validar_eeprom] sensor %d nao bateu: salvo(%d,%d) lido(%d,%d)\n",
                sensor, saved_max[sensor], saved_min[sensor], loaded_max[sensor], loaded_min[sensor]
            );
        }
    }

    Serial.println(ok ? "[validar_eeprom] OK, tudo bateu." : "[validar_eeprom] FALHOU, ver acima.");
}
