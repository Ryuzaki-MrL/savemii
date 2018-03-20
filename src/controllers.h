#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include "dynamic_libs/padscore_functions.h"
#include "dynamic_libs/vpad_functions.h"

enum buttons {
    PAD_BUTTON_A,
    PAD_BUTTON_B,
    PAD_BUTTON_X,
    PAD_BUTTON_Y,
    PAD_BUTTON_UP,
    PAD_BUTTON_DOWN,
    PAD_BUTTON_LEFT,
    PAD_BUTTON_RIGHT,
    PAD_BUTTON_L,
    PAD_BUTTON_R,
    PAD_BUTTON_ZL,
    PAD_BUTTON_ZR,
    PAD_BUTTON_PLUS,
    PAD_BUTTON_MINUS,
    PAD_BUTTON_Z,
    PAD_BUTTON_C,
    PAD_BUTTON_STICK_L,
    PAD_BUTTON_STICK_R,
    PAD_BUTTON_HOME,
    PAD_BUTTON_SYNC,
    PAD_BUTTON_TV,
    PAD_BUTTON_1,
    PAD_BUTTON_2,
    PAD_BUTTON_ANY
};

enum buttonStates {
    PRESS,
    HOLD,
    RELEASE
};

enum sticks {
    STICK_L = 0,
    STICK_R = 1,
    STICK_BOTH = 2
};

enum stickDirections {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_ANY
};

uint32_t buttons_hold[5]; // Held buttons
uint32_t buttons_pressed[5]; // Pressed buttons
uint32_t buttons_released[5]; // Released buttons
f32 stickPositions[5][2][2];
/* State of joysticks.
First index is controller, second is stick (L and R) and third is direction (X or Y)
These are to be read with checkStick(stick, stickDirection, threshold)*/

void pingControllers();
void updateControllers();
bool isWiimote(KPADData *padData);
bool hasNunchuck(KPADData *padData);
bool isClassicController(KPADData *padData);
bool isProController(KPADData *padData);
bool checkStick(u8 stick, u8 stickDirection, f32 threshold);
int checkButton(int button, int state);

#endif //CONTROLLERS_H
