# Phoenix RC-ROU

Firmware do robô seguidor de linha da equipe Phoenix para a competição
RC Romênia. Roda em ESP32-S3-WROOM-1-N16R8, framework Arduino, gerenciado
via PlatformIO.

Esse projeto é uma reconstrução enxuta do `bia-senna-code-2026`: mesma
base de ideias (PID + sensores AD7490 + Bluetooth), mas sem giroscópio,
sem encoder, sem contagem de marcadores/voltas e sem os sensores
laterais do robô antigo, que não existem neste chassi. O planejamento
completo, com todas as decisões e o histórico de hardware, está em
`../PLANEJAMENTO.md`.

## Hardware

- MCU: ESP32-S3-WROOM-1-N16R8
- Sensores de linha: 16 sensores frontais via ADC AD7490 (SPI, 12 bits, 16 canais - todos usados)
- Motores: 2 motores DC, cada um controlado por um par de chips BTN9960LV (meia-ponte, esquema sign-magnitude, sem estado de coast)
- Turbina de sucção: 1 canal PWM
- LEDs: 2 grupos FastLED independentes (frontal e principal)
- Bateria: LiPo 3S/4S
- Entrada de comando: Bluetooth (BLE) e Serial; infravermelho ainda não implementado

O pinout completo e a fonte de cada pino estão documentados em
`include/pinout.h`.

## Estrutura do projeto

```
firmware/
  include/
    config.h     configurações de comportamento (MODO_ROMENIA, failsafe)
    pinout.h     mapeamento de pinos, único lugar a editar quando um pino mudar
    utils.h      includes comuns
  lib/
    AD7490/          driver SPI do ADC dos sensores frontais
    battery/         leitura de tensão, status de bateria, failsafe de tensão
    bluetooth/       servidor BLE, protocolo de comando por string
    controllers/     PID de seguimento de linha
    eeprom_manager/  persistência da calibração dos sensores em EEPROM
    fan/             controle da turbina de sucção
    led/             controle dos 2 grupos de LED
    line_sensors/    leitura, normalização e calibração dos sensores frontais
    motors/          controle dos motores (BTN9960LV)
    state_machine/   integra tudo: máquina de estados e leitura de comandos
  src/
    main.cpp     setup() e loop()
```

Cada lib tem uma função `validar_<nome>()` para teste isolado de bancada,
disparável por comando sem precisar recompilar nada.

## Máquina de estados

O robô tem 3 estados:

- **Calibração**: estado inicial. Espera o comando de calibrar os sensores
  ou de já iniciar a corrida com a calibração salva anteriormente.
- **Corrida**: segue a linha com o PID até receber o comando de parar ou
  até o failsafe de linha perdida disparar.
- **Parado**: motores e turbina desligados. Aceita o comando de nova
  tentativa para voltar direto à calibração, sem precisar reiniciar a placa.

## Comandos

Comandos são recebidos por Bluetooth (prioridade) ou Serial, como strings
de 2 letras:

| Comando | Efeito |
|---|---|
| `KO` | Inicia calibração dos sensores frontais e salva na EEPROM |
| `ST` | Inicia a corrida |
| `SP` | Para a corrida |
| `EX` | Vai para o estado Parado |
| `RF` | Reset de fábrica da calibração salva (EEPROM e memória) |
| `TM` | Teste de motores |
| `TL` | Teste de LEDs |
| `TF` | Teste dos sensores frontais |
| `TN` | Teste da turbina |
| `TB` | Teste da bateria |
| `KP<v>` | Sobrescreve o ganho `kP` do PID pro próximo `ST` (ex.: `KP1.75`) |
| `KI<v>` | Sobrescreve o ganho `kI` do PID pro próximo `ST` |
| `KD<v>` | Sobrescreve o ganho `kD` do PID pro próximo `ST` |
| `MV<v>` | Sobrescreve a tensão base do motor (V) pro próximo `ST` |
| `FV<v>` | Define a tensão da turbina durante a corrida (V, 0 = desligada) |
| `PR` | Reseta `KP`/`KI`/`KD`/`MV`/`FV` pro padrão do código |

Os comandos de teste (`TM`, `TL`, `TF`, `TN`, `TB`) e os de ajuste
(`KP`–`FV`, `PR`) só são aceitos fora do estado de corrida (a confirmação
que os comandos de ajuste mandam de volta usa um `delay()` bloqueante,
que travaria o loop do PID se aceito no meio de uma tentativa). Os
comandos de ajuste só gravam o valor em memória, aplicado no próximo
`ST`; se nenhum for enviado, o robô usa os valores fixos do código
(mesmo comportamento de sempre). Nada é salvo em EEPROM — some se o
ESP32 desligar.

## Configuração antes de gravar

Duas coisas em `include/config.h` precisam ser conferidas antes de cada
sessão, bancada ou competição, e exigem regravar o ESP32 quando mudarem:

- `MODO_ROMENIA`: `true` para linha preta em fundo branco (padrão da
  competição), `false` para linha branca em fundo preto (padrão dos testes
  de bancada da equipe).
- `FAILSAFE_LINHA_PERDIDA`: liga ou desliga a parada automática quando o
  robô perde a linha por tempo demais.

## Compilando e gravando

```
pio run                 # compila
pio run -t upload       # grava no ESP32
pio device monitor       # abre o monitor serial (115200 baud)
```

## Estado atual

Todos os módulos estão implementados e testados, exceto o receptor de
infravermelho, que depende da confirmação de qual módulo/controle será
usado na competição. Até lá, o controle do robô é feito inteiramente por
Bluetooth ou Serial.
