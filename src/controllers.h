#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include <gctypes.h>
#include <padscore/kpad.h>
#include <padscore/wpad.h>
#include <vpad/input.h>

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

void pingControllers();
void updateButtons();
bool stickPos(u8 stick, f32 value);
bool isWiimote(KPADStatus *padData);
bool isClassicController(KPADStatus *padData);
bool isProController(KPADStatus *padData);
int checkButton(int button, int state);
// int isPressed(int button);
// int isHeld(int button);
// int isReleased(int button);

#endif //CONTROLLERS_H
