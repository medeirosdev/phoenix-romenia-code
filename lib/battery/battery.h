#ifndef __BATTERY_H__
#define __BATTERY_H__

#include "utils.h"

// =====================================================================
// Bateria: LiPo 3S ou 4S (confirmado por Ricardo, 21/07/2026 - o time usa
// o que estiver carregado/disponivel: 3S carrega cheia ~12,4V, 4S ~16,8V).
//
// Variavel global simples, mesmo padrao do MODO_ROMENIA (ver
// PLANEJAMENTO.md secao 7): trocar aqui e regravar quando a bateria em
// uso mudar de 3S pra 4S ou vice-versa.
// =====================================================================
#define BATTERY_CELL_COUNT 3   // TODO: confirmar qual pack esta no robo AGORA (3 ou 4)

// Faixas de tensao POR CELULA de uma LiPo comum (valores padrao da
// industria, nao dependem de calibracao de hardware):
#define VOLTS_PER_CELL_FULL      4.20  // cheia
#define VOLTS_PER_CELL_HIGH      3.90  // ainda com boa folga
#define VOLTS_PER_CELL_LOW       3.50  // hora de trocar/carregar
#define VOLTS_PER_CELL_CRITICAL  3.30  // NUNCA deixar chegar aqui, dana a celula

#define BATTERY_HIGH_THRESHOLD ((float)(BATTERY_CELL_COUNT) * VOLTS_PER_CELL_HIGH)
#define BATTERY_LOW_THRESHOLD  ((float)(BATTERY_CELL_COUNT) * VOLTS_PER_CELL_LOW)

// Parametros de calibracao do ADC: NAO SAO OS DO PROJETO ANTIGO (a placa
// antiga sensoreava so ate ~12,6V; essa placa precisa sensoriar ate
// ~16,8V pra aguentar 4S, entao o divisor de tensao e outro). Os valores
// abaixo sao um placeholder generico e QUASE CERTAMENTE ERRADOS pra essa
// placa - precisam ser recalibrados na bancada com um multimetro antes
// de confiar na leitura de tensao (ver validar_bateria()).
#define BATTERY_SAMPLE_READINGS     4       // quantidade de leituras por amostra (reduz ruido)
#define BATTERY_VOLTAGE_PARAMETER   16.80   // TODO: recalibrar - tensao medida no multimetro
#define BATTERY_ADC_PARAMETER       4095    // TODO: recalibrar - leitura do ADC correspondente

// Abaixo disso, consideramos que nao tem bateria de verdade conectada (pino
// de leitura flutuando/proximo de 0V) - e uma caracteristica do CIRCUITO de
// leitura, nao da bateria, entao NAO escala com BATTERY_CELL_COUNT (isso
// era um bug: uma versao anterior multiplicava esse valor pelas celulas,
// o que nao faz sentido - o circuito nao sabe quantas celulas a bateria
// ausente teria). Tambem e placeholder ate calibrar na bancada.
#define USB_THRESHOLD 3.0 // TODO: recalibrar

// Piso de seguranca: nunca dividir por uma tensao de bateria menor que essa
// (evita divisao por zero/quase-zero em set_motor_voltage()/set_fan_voltage()
// se a bateria estiver desconectada ou a leitura falhar).
#define MIN_BATTERY_VOLTAGE_FOR_PWM 1.0

// Tempo (ms) de bateria fraca ininterrupta antes de battery_failsafe_triggered()
// comecar a retornar true. O que fazer com isso ainda e uma decisao em
// aberto (ver PLANEJAMENTO.md secao 12) - essa lib so mede e avisa, nao
// desliga nada sozinha.
#define FAILSAFE_TIMEOUT 4500

enum Battery_Status {
    BATTERY_HIGH,
    BATTERY_MEDIUM,
    BATTERY_LOW,
    USB_POWER
};

void battery_init();
void battery_monitoring();
Battery_Status get_battery_status();
float get_battery_voltage();
bool battery_failsafe_triggered();
void validar_bateria();

#endif /* __BATTERY_H__ */
