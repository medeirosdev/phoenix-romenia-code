#include "utils.h"
#include "pinout.h"
#include "battery.h"
#include "motors.h"
#include "fan.h"
#include "line_sensors.h"
#include "controllers.h"

// Estagio de bring-up: so inicializa e valida os modulos ja prontos
// (battery, motors, fan, line_sensors, controllers). Ainda NAO existe
// maquina de estados - isso e temporario, vai ser substituido quando o
// state_machine (secao 13, passo 6 do PLANEJAMENTO.md) existir.
// Comentar/descomentar as chamadas de validar_*() conforme o que
// estiver testando na bancada - as validacoes de bring-up (bateria,
// motores, fan, sensores) ficam comentadas por padrao pra nao atrasar
// em ~35s todo boot enquanto o foco e testar o seguidor de linha.

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Phoenix Romenia - firmware iniciado (bring-up)");

    battery_init();
    motors_init();
    fan_init();
    line_sensors_init();

    // validar_bateria();
    // validar_motores();
    // validar_fan();
    // validar_sensores_frontais();

    // NUNCA RETORNA - fica seguindo a linha pra sempre. Colocar o robo
    // NA LINHA antes de ligar (ver aviso completo em controllers.cpp).
    validar_controllers();
}

void loop() {
}
