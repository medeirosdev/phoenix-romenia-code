#include "utils.h"
#include "pinout.h"

// Esqueleto inicial do projeto - ainda sem nenhuma lib (motors, battery,
// etc.) integrada. So confirma que o board builda e liga.
// Ver codigo_romenia/PLANEJAMENTO.md secao 13 pra ordem de implementacao.

void setup() {
    Serial.begin(115200);
    Serial.println("Phoenix Romenia - firmware iniciado (esqueleto)");
}

void loop() {
}
