#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__

#include "utils.h"

// 3 estados, conforme PLANEJAMENTO.md secao 5 - sem os outros 6 estados
// do projeto antigo (giroscopio, mapa, chaser, etc. nao existem aqui).
enum Robot_State {
    CALIBRATION_STATE,
    RACE_STATE,
    STOPPED_STATE
};

class Robot {
    public:
        Robot_State current_state;

        void init();
        void run();

    private:
        // 0 = robo esta na linha agora (ou corrida nao comecou ainda);
        // != 0 = millis() de quando ele saiu da linha, pro failsafe de
        // "saiu da linha" (config.h) contar o tempo corrido.
        unsigned long line_lost_since = 0;

        void calibration_state();
        void race_state();
        void stopped_state();
        void set_state(Robot_State new_state);
};

extern Robot robot;

#endif /* __STATE_MACHINE_H__ */
