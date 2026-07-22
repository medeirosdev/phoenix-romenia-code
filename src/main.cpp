#include "utils.h"
#include "pinout.h"
#include "battery.h"
#include "motors.h"
#include "fan.h"

// Estagio de bring-up: so inicializa e valida os modulos ja prontos
// (battery, motors, fan). Ainda NAO existe maquina de estados - isso e
// temporario, vai ser substituido quando o state_machine (secao 13,
// passo 6 do PLANEJAMENTO.md) existir. Comentar/descomentar as chamadas
// de validar_*() conforme o que estiver testando na bancada.

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Phoenix Romenia - firmware iniciado (bring-up)");

    battery_init();
    motors_init();
    fan_init();

    validar_bateria();
    validar_motores();
    validar_fan();

    Serial.println("Bring-up concluido.");
}

void loop() {
}
