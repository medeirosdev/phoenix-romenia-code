#include "state_machine.h"
#include "line_sensors.h"
#include "controllers.h"
#include "motors.h"
#include "fan.h"

// =====================================================================
// Entrada do usuario: ainda NAO existe IR nem Bluetooth (isso e passo 7
// e 8 do PLANEJAMENTO.md secao 13 - vem DEPOIS do state_machine, que e
// o passo 6). Por enquanto os comandos vem digitados no Serial Monitor
// (uma linha por comando): KO, ST, SP, EX - mesmos codigos ja
// documentados na secao 8 do PLANEJAMENTO.md.
//
// Essa funcao vai ser SUBSTITUIDA pelo user_interface de verdade (merge
// IR+BT) quando esses modulos existirem - a ideia e so trocar de onde o
// comando vem, sem mudar a logica de estados abaixo.
// =====================================================================
enum User_Command {
    COMMAND_NONE,
    COMMAND_START_CALIBRATION, // KO
    COMMAND_START_RACE,        // ST
    COMMAND_STOP,               // SP
    COMMAND_EXIT                 // EX
};

static User_Command read_user_input() {
    if (!Serial.available()) return COMMAND_NONE;

    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line == "KO") return COMMAND_START_CALIBRATION;
    if (line == "ST") return COMMAND_START_RACE;
    if (line == "SP") return COMMAND_STOP;
    if (line == "EX") return COMMAND_EXIT;

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
// (decisao do usuario, PLANEJAMENTO.md secao 10) - a UNICA forma de sair
// daqui e o comando de STOP.
void Robot::race_state() {
    line_pid.run();

    if (read_user_input() == COMMAND_STOP) {
        Serial.println("[state_machine] STOP recebido.");
        set_state(STOPPED_STATE);
    }
}

// Estado terminal: freia e desliga tudo. So sai daqui com reset fisico -
// esse e o comportamento ASSUMIDO enquanto a pergunta da secao 12 (item
// sobre o estado SAIR aceitar "nova tentativa" ou nao) nao e respondida.
void Robot::stopped_state() {
    static bool announced = false;
    if (!announced) {
        brake_motors(true);
        set_fan_voltage(0);
        Serial.println("[state_machine] Parado. So volta a rodar com reset fisico da placa.");
        announced = true;
    }
}
