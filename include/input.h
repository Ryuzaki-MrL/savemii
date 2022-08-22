#pragma once

#include <padscore/kpad.h>
#include <vpad/input.h>
#include <cstring>

extern VPADStatus vpad_status;
extern VPADReadError vpad_error;
extern KPADStatus kpad[4], kpad_status;

typedef enum Button {
    PAD_BUTTON_A,
    PAD_BUTTON_B,
    PAD_BUTTON_UP,
    PAD_BUTTON_DOWN,
    PAD_BUTTON_LEFT,
    PAD_BUTTON_RIGHT,
    PAD_BUTTON_L,
    PAD_BUTTON_R,
    PAD_BUTTON_ANY
} Button;

typedef enum ButtonState {
    TRIGGER,
    HOLD,
    RELEASE
} ButtonState;

void readInput() __attribute__((hot));
bool getInput(ButtonState state, Button button) __attribute__((hot));
