#include "state_machine.h"
#include "line_sensors.h"
#include "controllers.h"
#include "motors.h"
#include "fan.h"
#include "bluetooth.h"
#include "battery.h"
#include "led.h"
#include "eeprom_manager.h"
#include "config.h"

// =====================================================================
// Entrada do usuario: Bluetooth ja existe (passo 7), mas IR ainda nao
// (passo 8, PLANEJAMENTO.md secao 13). Aceita comando tanto por
// Bluetooth quanto por Serial Monitor (o Serial fica como atalho de
// bancada, util quando nao da pra conectar um celular por perto) - mesmos
// codigos documentados na secao 8: KO, ST, SP, EX, os de diagnostico
// TM/TL/TF/TN/TB (decisao do usuario, 22/07/2026: comando via BT, igual
// o padrao do projeto antigo - opcao 1 da PLANEJAMENTO.md secao 12), e
// RF (reset de fabrica da calibracao salva, PLANEJAMENTO.md secao 9).
//
// Essa funcao vai virar o user_interface de verdade (merge IR+BT) quando
// o IR existir - a ideia e so acrescentar mais uma fonte, sem mudar a
// logica de estados abaixo.
// =====================================================================
enum User_Command {
    COMMAND_NONE,
    COMMAND_START_CALIBRATION, // KO
    COMMAND_START_RACE,        // ST
    COMMAND_STOP,               // SP
    COMMAND_EXIT,                 // EX
    COMMAND_TEST_MOTORS,           // TM
    COMMAND_TEST_LED,               // TL
    COMMAND_TEST_FRONTAL_SENSORS,    // TF
    COMMAND_TEST_FAN,                 // TN
    COMMAND_TEST_BATTERY,               // TB
    COMMAND_RESET_CALIBRATION            // RF
};

static User_Command parse_command(const String &command) {
    if (command == "KO") return COMMAND_START_CALIBRATION;
    if (command == "ST") return COMMAND_START_RACE;
    if (command == "SP") return COMMAND_STOP;
    if (command == "EX") return COMMAND_EXIT;
    if (command == "TM") return COMMAND_TEST_MOTORS;
    if (command == "TL") return COMMAND_TEST_LED;
    if (command == "TF") return COMMAND_TEST_FRONTAL_SENSORS;
    if (command == "TN") return COMMAND_TEST_FAN;
    if (command == "TB") return COMMAND_TEST_BATTERY;
    if (command == "RF") return COMMAND_RESET_CALIBRATION;
    return COMMAND_NONE;
}

static User_Command read_user_input() {
    String bluetooth_command = read_bluetooth_message();
    if (bluetooth_command != "") return parse_command(bluetooth_command);

    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        return parse_command(line);
    }

    return COMMAND_NONE;
}

// Comandos de diagnostico (TM/TL/TF/TN/TB, PLANEJAMENTO.md secao 8) -
// disponiveis em CALIBRACAO e SAIR, mas NAO em RACE_STATE: rodar um
// validar_*() no meio da corrida ia brigar com o PID escrevendo nos
// motores ao mesmo tempo. Cada validar_*() e bloqueante (alguns
// segundos) - normal pra um comando de bancada, nao de corrida. Retorna
// true se tratou o comando (pra quem chamou saber que nao precisa
// processar mais nada nesse ciclo).
static bool handle_diagnostic_command(User_Command command) {
    switch (command) {
        case COMMAND_TEST_MOTORS:          validar_motores();           return true;
        case COMMAND_TEST_LED:             validar_led();               return true;
        case COMMAND_TEST_FRONTAL_SENSORS: validar_sensores_frontais(); return true;
        case COMMAND_TEST_FAN:             validar_fan();               return true;
        case COMMAND_TEST_BATTERY:         validar_bateria();           return true;
        default:                           return false;
    }
}

Robot robot;

void Robot::init() {
    load_line_sensors_calibration(); // se nao tiver nada salvo, so mantem o default
    current_state = CALIBRATION_STATE;
}

void Robot::set_state(Robot_State new_state) {
    current_state = new_state;
}

void Robot::run() {
    // battery_monitoring() precisa rodar sempre, nao so dentro de
    // validar_bateria() - sem isso, get_battery_voltage() fica travada no
    // valor inicial otimista pra sempre, e a compensacao de tensao em
    // set_motor_voltage()/set_fan_voltage() nunca reflete a bateria real
    // (bug encontrado em revisao de codigo, 22/07/2026).
    battery_monitoring();
    bluetooth_check_connection();

    switch (current_state) {
        case CALIBRATION_STATE: calibration_state(); break;
        case RACE_STATE:        race_state();        break;
        case STOPPED_STATE:     stopped_state();      break;
    }
}

// Espera comando de calibrar (roda a calibracao de 5s, bloqueante - ver
// PLANEJAMENTO.md secao 5 sobre essa limitacao ja conhecida: nao da pra
// abortar no meio) ou de ja iniciar a corrida com o que estiver
// carregado (default de fabrica ou calibracao anterior).
void Robot::calibration_state() {
    User_Command command = read_user_input();
    if (handle_diagnostic_command(command)) return;

    switch (command) {
        case COMMAND_START_CALIBRATION:
            Serial.println("[state_machine] Calibrando sensores frontais...");
            calibrate_line_sensors();
            save_line_sensors_calibration(); // salva automaticamente - sem os 3 modos do projeto antigo
            Serial.println("[state_machine] Calibracao concluida e salva. Envie ST pra iniciar a corrida.");
            break;

        case COMMAND_START_RACE:
            Serial.println("[state_machine] Iniciando corrida.");
            controllers_init();
            line_lost_since = 0; // zera o cronometro do failsafe pra essa tentativa
            set_state(RACE_STATE);
            break;

        case COMMAND_EXIT:
            set_state(STOPPED_STATE);
            break;

        case COMMAND_RESET_CALIBRATION:
            erase_line_sensors_calibration();
            break;

        default:
            break;
    }
}

// So anda: segue a linha com o PID, sem contar marcador/cruzamento/volta
// (decisao do usuario, PLANEJAMENTO.md secao 10) - sai daqui com o
// comando de STOP, ou sozinho se o failsafe de "saiu da linha" disparar
// (config.h - FAILSAFE_LINHA_PERDIDA).
void Robot::race_state() {
    line_pid.run();

    if (FAILSAFE_LINHA_PERDIDA) {
        if (line_sensors_is_on_line()) {
            line_lost_since = 0;
        } else {
            if (line_lost_since == 0) line_lost_since = millis();

            if (millis() - line_lost_since >= FAILSAFE_LINHA_PERDIDA_TIMEOUT_MS) {
                Serial.println("[state_machine] Failsafe: linha perdida ha muito tempo, parando.");
                set_state(STOPPED_STATE);
                return;
            }
        }
    }

    if (read_user_input() == COMMAND_STOP) {
        Serial.println("[state_machine] STOP recebido.");
        set_state(STOPPED_STATE);
    }
}

// Estado "parado": freia e desliga tudo. Aceita KO pra voltar direto pra
// CALIBRACAO (nova tentativa, sem precisar resetar a placa - decisao do
// usuario, PLANEJAMENTO.md secao 12) ou fica parado esperando esse
// comando indefinidamente.
void Robot::stopped_state() {
    static bool announced = false;
    if (!announced) {
        brake_motors(true);
        set_fan_voltage(0);
        Serial.println("[state_machine] Parado. Envie KO pra uma nova tentativa.");
        announced = true;
    }

    User_Command command = read_user_input();
    if (handle_diagnostic_command(command)) return;

    if (command == COMMAND_START_CALIBRATION) {
        Serial.println("[state_machine] Nova tentativa - voltando pra calibracao.");
        announced = false;
        set_state(CALIBRATION_STATE);
    }
}
