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

uint32_t buttons_hold[5]; //Held buttons
uint32_t buttons_pressed[5]; //Pressed buttons
uint32_t buttons_released[5]; //Released buttons

void pingControllers();
void updatePressedButtons();
void updateHeldButtons();
void updateReleasedButtons();
bool stickPos(u8 stick, f32 value);
bool isWiimote(KPADData *padData);
bool isClassicController(KPADData *padData);
bool isProController(KPADData *padData);
int isPressed(int button);
int isHeld(int button);
int isReleased(int button);

#endif //CONTROLLERS_H
