//Based on code from lib_easy, current stickPos() is straight borrowed from there

#include "controllers.h"

int vpadError = -1;
VPADStatus vpad;

s32 padErrors[4];
u32 padTypes[4];
KPADStatus pads[4];

uint32_t buttons_hold[5]; //Held buttons
uint32_t buttons_pressed[5]; //Pressed buttons
uint32_t buttons_released[5]; //Released buttons

void pingControllers() {
    for (int i = 0; i < 4; i++) {
        padErrors[i] = WPADProbe(i, &padTypes[i]);
    }
}

void updateButtons() {
    VPADRead(0, &vpad, 1, &vpadError);
    buttons_pressed[0] = vpad.trigger;
    buttons_hold[0] = vpad.hold;
    buttons_released[0] = vpad.release;

    pingControllers();
    for (int i = 0; i < 4; i++) {
        if (padErrors[i] == 0) {
            KPADRead(i, &pads[i], 1);
            if (isWiimote(&pads[i])) {
                buttons_pressed[i + 1] = pads[i].trigger;
                buttons_hold[i + 1] = pads[i].hold;
                buttons_released[i + 1] = pads[i].release;
            }
            else if (isClassicController(&pads[i])) {
                buttons_pressed[i + 1] = pads[i].classic.trigger;
                buttons_hold[i + 1] = pads[i].classic.hold;
                buttons_released[i + 1] = pads[i].classic.release;
            }
            else if (isProController(&pads[i])) {
                buttons_pressed[i + 1] = pads[i].pro.trigger;
                buttons_hold[i + 1] = pads[i].pro.hold;
                buttons_released[i + 1] = pads[i].pro.release;
            }
        }
    }
}

bool stickPos(u8 stick, f32 value) {
    switch(stick) {
        case 0 :
            return (value > 0) ? (vpad.leftStick.x > value): (vpad.leftStick.x < value);
        case 1 :
            return (value > 0) ? (vpad.leftStick.y > value): (vpad.leftStick.y < value);
        case 2 :
            return (value > 0) ? (vpad.rightStick.x > value): (vpad.rightStick.x < value);
        case 3 :
            return (value > 0) ? (vpad.rightStick.y > value): (vpad.rightStick.y < value);
        case 4 :
            return ((vpad.leftStick.x > value) || (vpad.leftStick.x < -value)) || \
                   ((vpad.leftStick.y > value) || (vpad.leftStick.y < -value)) || \
                   ((vpad.rightStick.x > value) || (vpad.rightStick.x < -value)) || \
                   ((vpad.rightStick.y > value) || (vpad.rightStick.y < -value));

        default :
            return 0;
    }
}

bool isWiimote(KPADStatus *padData){
    return padData->extensionType == 0 || padData->extensionType == 1 || padData->extensionType == 5 || padData->extensionType == 6;
}

bool isClassicController(KPADStatus *padData){
    return padData->extensionType == 2 || padData->extensionType == 7;
}

bool isProController(KPADStatus *padData){
    return padData->extensionType == 31;
}

int checkButton(int button, int state) {
    uint32_t *stateArray;

    switch(state) {
        case PRESS:
            stateArray = buttons_pressed;
            break;

        case HOLD:
            stateArray = buttons_hold;
            break;

        case RELEASE:
            stateArray = buttons_released;
            break;

        default:
            return 0;
    }

    //Check for any button at all
    if (button == PAD_BUTTON_ANY) {
        for (int i = 0; i < 5; i++) {
            if (stateArray[i] > 0) return 1;
        }
    }

    //VPad buttons
    switch (button) {
        case PAD_BUTTON_A:
            if (stateArray[0] & VPAD_BUTTON_A) return 1;
            break;

        case PAD_BUTTON_B:
            if (stateArray[0] & VPAD_BUTTON_B) return 1;
            break;

        case PAD_BUTTON_X:
            if (stateArray[0] & VPAD_BUTTON_X) return 1;
            break;

        case PAD_BUTTON_Y:
            if (stateArray[0] & VPAD_BUTTON_Y) return 1;
            break;

        case PAD_BUTTON_UP:
            if (stateArray[0] & VPAD_BUTTON_UP) return 1;
            break;

        case PAD_BUTTON_DOWN:
            if (stateArray[0] & VPAD_BUTTON_DOWN) return 1;
            break;

        case PAD_BUTTON_LEFT:
            if (stateArray[0] & VPAD_BUTTON_LEFT) return 1;
            break;

        case PAD_BUTTON_RIGHT:
            if (stateArray[0] & VPAD_BUTTON_RIGHT) return 1;
            break;

        case PAD_BUTTON_L:
            if (stateArray[0] & VPAD_BUTTON_L) return 1;
            break;

        case PAD_BUTTON_R:
            if (stateArray[0] & VPAD_BUTTON_R) return 1;
            break;

        case PAD_BUTTON_ZL:
            if (stateArray[0] & VPAD_BUTTON_ZL) return 1;
            break;

        case PAD_BUTTON_ZR:
            if (stateArray[0] & VPAD_BUTTON_ZR) return 1;
            break;

        case PAD_BUTTON_PLUS:
            if (stateArray[0] & VPAD_BUTTON_PLUS) return 1;
            break;

        case PAD_BUTTON_MINUS:
            if (stateArray[0] & VPAD_BUTTON_MINUS) return 1;
            break;

        case PAD_BUTTON_HOME:
            if (stateArray[0] & VPAD_BUTTON_HOME) return 1;
            break;

        case PAD_BUTTON_SYNC:
            if (stateArray[0] & VPAD_BUTTON_SYNC) return 1;
            break;

        case PAD_BUTTON_STICK_L:
            if (stateArray[0] & VPAD_BUTTON_L) return 1;
            break;

        case PAD_BUTTON_STICK_R:
            if (stateArray[0] & VPAD_BUTTON_STICK_R) return 1;
            break;

        case PAD_BUTTON_TV:
            if (stateArray[0] & VPAD_BUTTON_TV) return 1;
            break;

        default:
            break;
    }

    //Buttons handled by the padscore library
    for (int i = 0; i < 4; i++) {
        if (padErrors[i] == 0) {
            if (isWiimote(&pads[i])) {
                switch (button) {
                    case PAD_BUTTON_UP:
                        if (stateArray[i + 1] & WPAD_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (stateArray[i + 1] & WPAD_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (stateArray[i + 1] & WPAD_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (stateArray[i + 1] & WPAD_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (stateArray[i + 1] & WPAD_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (stateArray[i + 1] & WPAD_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (stateArray[i + 1] & WPAD_BUTTON_1) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (stateArray[i + 1] & WPAD_BUTTON_2) return 1;
                        break;

                    case PAD_BUTTON_1:
                        if (stateArray[i + 1] & WPAD_BUTTON_1) return 1;
                        break;

                    case PAD_BUTTON_2:
                        if (stateArray[i + 1] & WPAD_BUTTON_2) return 1;
                        break;

                    case PAD_BUTTON_Z:
                        if (stateArray[i + 1] & WPAD_BUTTON_Z) return 1;
                        break;

                    case PAD_BUTTON_C:
                        if (stateArray[i + 1] & WPAD_BUTTON_C) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (stateArray[i + 1] & WPAD_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (stateArray[i + 1] & WPAD_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (stateArray[i + 1] & WPAD_BUTTON_HOME) return 1;
                        break;
                }
            }
            //Turns out the Pro Controller and Classic Controller have almost the exact same mapping
            //Except for the Pro Controller having clicky sticks
            else if (isClassicController(&pads[i]) || isProController(&pads[i])) {
                switch (button) {
                    case PAD_BUTTON_UP:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_X:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_X) return 1;
                        break;

                    case PAD_BUTTON_Y:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_Y) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_L) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_R) return 1;
                        break;

                    case PAD_BUTTON_ZL:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_ZL) return 1;
                        break;

                    case PAD_BUTTON_ZR:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_ZR) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (stateArray[i + 1] & WPAD_CLASSIC_BUTTON_HOME) return 1;
                        break;
                }
                //Here, we handle the aforementioned clicky sticks
                if (isProController(&pads[i])) {
                    switch (button) {
                        case PAD_BUTTON_STICK_L:
                            if (stateArray[i + 1] & WPAD_PRO_BUTTON_STICK_L) return 1;
                            break;

                        case PAD_BUTTON_STICK_R:
                            if (stateArray[i + 1] & WPAD_PRO_BUTTON_STICK_R) return 1;
                            break;
                    }
                }
            }
        }
    }

    return 0;
}
