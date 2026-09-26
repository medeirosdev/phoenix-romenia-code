#ifndef __CONFIG_H__
#define __CONFIG_H__

// =====================================================================
// Configuracoes gerais do robo: variaveis simples pra trocar no codigo e
// regravar, em vez de comando em tempo de execucao (decisao do usuario,
// ver PLANEJAMENTO.md secao 7). Diferente de pinout.h, que e so sobre
// numero de pino - aqui entra comportamento.
// =====================================================================

// true  = linha PRETA em fundo BRANCO (padrao da competicao Romenia)
// false = linha BRANCA em fundo PRETO (padrao dos nossos testes de bancada)
//
// Trocar esse valor e regravar o ESP32 antes de cada sessao (bancada vs
// competicao) - conferir sempre antes de embarcar.
#define MODO_ROMENIA true

// Failsafe "saiu da linha": se nenhum sensor frontal detectar a linha por
// FAILSAFE_LINHA_PERDIDA_TIMEOUT_MS seguidos durante a corrida, o robo
// para sozinho (freia e vai pro estado SAIR). Liga/desliga aqui, mesmo
// padrao do MODO_ROMENIA (PLANEJAMENTO.md secao 12).
#define FAILSAFE_LINHA_PERDIDA true
#define FAILSAFE_LINHA_PERDIDA_TIMEOUT_MS 500 // ponto de partida, ajustar na pista

// Rampa de partida: com PASSO_MOTOR_LIGADO true, a tensao base dos motores
// comeca em 0 a cada ST e sobe TAMANHO_PASSO volts por segundo ate chegar
// na tensao configurada (LINE_PID_BASE_VOLTAGE ou MV), em vez de ja sair
// com 100% dela. Com false, sai direto na tensao cheia (comportamento
// antigo).
#define PASSO_MOTOR_LIGADO true
#define TAMANHO_PASSO 2.0 // V/s - ex.: base de 10V leva 5s pra chegar no total

// Rampa da turbina: com PASSO_TURBINA_LIGADO true, a cada ST o robo fica
// PARADO enquanto a turbina sobe de 0 ate a tensao configurada (FV),
// TAMANHO_PASSO_TURBINA volts por segundo. Os motores so saem quando a
// turbina chega a MARGEM_TURBINA_PARTIDA volts do alvo (ex.: alvo 10V,
// margem 1V -> sai em 9V); a turbina continua subindo ate 10V com o robo
// ja andando. A rampa dos motores (PASSO_MOTOR_LIGADO) comeca nesse
// momento, nao no ST. Com false, turbina liga direto no valor cheio e o
// robo sai na hora.
#define PASSO_TURBINA_LIGADO true
#define TAMANHO_PASSO_TURBINA 2.0   // V/s
#define MARGEM_TURBINA_PARTIDA 1.0  // V abaixo do alvo em que os motores liberam

#endif /* __CONFIG_H__ */
