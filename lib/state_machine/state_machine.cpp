#include "state_machine.h"
#include "line_sensors.h"
#include "controllers.h"
#include "motors.h"
#include "fan.h"
#include "bluetooth.h"
#include "config.h"

// =====================================================================
// Entrada do usuario: Bluetooth ja existe (passo 7), mas IR ainda nao
// (passo 8, PLANEJAMENTO.md secao 13). Aceita comando tanto por
// Bluetooth quanto por Serial Monitor (o Serial fica como atalho de
// bancada, util quando nao da pra conectar um celular por perto) - mesmos
// codigos documentados na secao 8: KO, ST, SP, EX.
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
    COMMAND_EXIT                 // EX
};

static User_Command parse_command(const String &command) {
    if (command == "KO") return COMMAND_START_CALIBRATION;
    if (command == "ST") return COMMAND_START_RACE;
    if (command == "SP") return COMMAND_STOP;
    if (command == "EX") return COMMAND_EXIT;
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

Robot robot;

void Robot::init() {
    current_state = CALIBRATION_STATE;
}

void Robot::set_state(Robot_State new_state) {
    current_state = new_state;
}

void Robot::run() {
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
    switch (read_user_input()) {
        case COMMAND_START_CALIBRATION:
            Serial.println("[state_machine] Calibrando sensores frontais...");
            calibrate_line_sensors();
            Serial.println("[state_machine] Calibracao concluida. Envie ST pra iniciar a corrida.");
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

    if (read_user_input() == COMMAND_START_CALIBRATION) {
        Serial.println("[state_machine] Nova tentativa - voltando pra calibracao.");
        announced = false;
        set_state(CALIBRATION_STATE);
    }
}
