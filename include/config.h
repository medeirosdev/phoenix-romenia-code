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

#endif /* __CONFIG_H__ */
