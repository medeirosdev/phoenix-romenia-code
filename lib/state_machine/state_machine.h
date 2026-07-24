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
        void calibration_state();
        void race_state();
        void stopped_state();
        void set_state(Robot_State new_state);
};

extern Robot robot;

#endif /* __STATE_MACHINE_H__ */
