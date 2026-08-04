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

// Comandos de PARAMETRO (KP/KI/KD/MV/FV/PR/PS, PLANEJAMENTO.md secao 8) -
// diferente dos comandos fixos acima, esses carregam um valor junto
// (ex.: "KP1.75") ou so consultam/resetam (PS/PR), por isso sao tratados
// aqui, ANTES de parse_command() virar um User_Command - nao mudam de
// estado, so gravam o valor em RAM pra o PROXIMO ST usar
// (controllers.h/fan.h).
//
// SO chamados em CALIBRACAO/SAIR (igual handle_diagnostic_command(), ver
// read_user_input_with_params() abaixo) - NAO em RACE_STATE. Motivo:
// manda confirmacao por Bluetooth, e send_bluetooth_message() tem um
// delay(10) bloqueante - aceitavel fora da corrida, mas um comando KP/MV
// chegando no meio de uma tentativa travaria o loop principal por 10ms
// bem em cima do PID (sampling_rate_ms=2, ver controllers.h). Como o
// valor so aplica no PROXIMO ST de qualquer jeito, nao tem motivo real
// pra aceitar durante a corrida.
static bool handle_param_command(const String &command) {
    if (command == "PR") {
        reset_pid_overrides();
        reset_race_fan_voltage();
        Serial.println("[state_machine] Parametros de teste resetados pro padrao do codigo.");
        send_bluetooth_message("PR ok - parametros resetados");
        return true;
    }
    if (command == "PS") {
        // Consulta os valores REAIS em uso agora (default do codigo, ou
        // sobrescrito por KP/KD/MV/FV) - util pra quem conectar depois de
        // outra pessoa ja ter mudado algo, ou depois de um reboot do
        // ESP32 (que perde os overrides, ja que nao sao salvos em EEPROM).
        String status = "KP=" + String(get_pid_kp(), 3)
                       + " KD=" + String(get_pid_kd(), 3)
                       + " MV=" + String(get_motor_base_voltage(), 2) + "V"
                       + " FV=" + String(get_race_fan_voltage(), 2) + "V";
        Serial.println("[state_machine] " + status);
        send_bluetooth_message(status);
        return true;
    }
    if (command.startsWith("KP")) {
        float value = command.substring(2).toFloat();
        set_pid_kp(value);
        send_bluetooth_message("KP=" + String(value, 3) + " (aplica no proximo ST)");
        return true;
    }
    if (command.startsWith("KI")) {
        float value = command.substring(2).toFloat();
        set_pid_ki(value);
        send_bluetooth_message("KI=" + String(value, 3) + " (aplica no proximo ST)");
        return true;
    }
    if (command.startsWith("KD")) {
        float value = command.substring(2).toFloat();
        set_pid_kd(value);
        send_bluetooth_message("KD=" + String(value, 3) + " (aplica no proximo ST)");
        return true;
    }
    if (command.startsWith("MV")) {
        float value = command.substring(2).toFloat();
        set_motor_base_voltage(value);
        send_bluetooth_message("MV=" + String(value, 2) + "V (aplica no proximo ST)");
        return true;
    }
    if (command.startsWith("FV")) {
        float value = command.substring(2).toFloat();
        set_race_fan_voltage(value);
        send_bluetooth_message("FV=" + String(value, 2) + "V (aplica no proximo ST)");
        return true;
    }
    return false;
}

static String read_raw_command() {
    String bluetooth_command = read_bluetooth_message();
    if (bluetooth_command != "") return bluetooth_command;

    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        return line;
    }

    return "";
}

// Usado em RACE_STATE - nao tenta interpretar comando de parametro (ver
// handle_param_command() acima pro motivo). Uma string tipo "KP1.5" so
// cai no default de parse_command() (COMMAND_NONE) e e ignorada, mesmo
// comportamento de antes desses comandos existirem.
static User_Command read_user_input() {
    String raw = read_raw_command();
    if (raw == "") return COMMAND_NONE;
    return parse_command(raw);
}

// Usado em CALIBRACAO/SAIR - tenta comando de parametro primeiro.
static User_Command read_user_input_with_params() {
    String raw = read_raw_command();
    if (raw == "") return COMMAND_NONE;
    if (handle_param_command(raw)) return COMMAND_NONE;
    return parse_command(raw);
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
    User_Command command = read_user_input_with_params();
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
            // Liga a turbina no valor configurado (FV) - 0 (padrao) mantem
            // o comportamento de sempre: turbina desligada durante a
            // corrida, ate o time decidir testar isso via app.
            set_fan_voltage(get_race_fan_voltage());
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

    User_Command command = read_user_input_with_params();
    if (handle_diagnostic_command(command)) return;

    if (command == COMMAND_START_CALIBRATION) {
        Serial.println("[state_machine] Nova tentativa - voltando pra calibracao.");
        announced = false;
        set_state(CALIBRATION_STATE);
    }
}
