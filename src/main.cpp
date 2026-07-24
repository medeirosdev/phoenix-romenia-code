#include "utils.h"
#include "pinout.h"
#include "battery.h"
#include "motors.h"
#include "fan.h"
#include "line_sensors.h"
#include "controllers.h"
#include "bluetooth.h"
#include "state_machine.h"

// As validacoes de bring-up (bateria, motores, fan, sensores, bluetooth)
// ficam comentadas por padrao - descomentar conforme o que estiver
// testando na bancada (cada uma delas bloqueia o boot por um tempo).
//
// Entrada do usuario: Bluetooth (app/terminal BLE) ou Serial Monitor
// como atalho de bancada (KO/ST/SP/EX) - ver aviso completo em
// state_machine.cpp. IR ainda falta (passo 8 do PLANEJAMENTO.md).

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Phoenix Romenia - firmware iniciado");

    battery_init();
    motors_init();
    fan_init();
    line_sensors_init();
    bluetooth_init();

    // validar_bateria();
    // validar_motores();
    // validar_fan();
    // validar_sensores_frontais();
    // validar_bluetooth();
    // validar_controllers();

    robot.init();
    Serial.println("Pronto. Envie KO (calibrar), ST (iniciar corrida), SP (parar) ou EX (sair) por Bluetooth ou Serial.");
}

void loop() {
    robot.run();
}
