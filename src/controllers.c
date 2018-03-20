//Based on code from lib_easy by rw-r-r-0644

#include "controllers.h"

s32 vpadError = -1;
VPADData vpad;

s32 padErrors[4];
u32 padTypes[4];
KPADData pads[4];

void pingControllers() {
    for (int i = 0; i < 4; i++) {
        padErrors[i] = WPADProbe(i, &padTypes[i]);
    }
}

void updateControllers() {
    VPADRead(0, &vpad, 1, &vpadError);
    buttons_pressed[0] = vpad.btns_d;
    buttons_hold[0] = vpad.btns_h;
    buttons_released[0] = vpad.btns_r;

    stickPositions[0][0][0] = vpad.lstick.x;
    stickPositions[0][0][1] = vpad.lstick.y;
    stickPositions[0][1][0] = vpad.rstick.x;
    stickPositions[0][1][1] = vpad.rstick.y;

    pingControllers();
    for (int i = 0; i < 4; i++) {
        if (padErrors[i] == 0) {
            KPADRead(i, &pads[i], 1);
            if (isWiimote(&pads[i])) {
                buttons_pressed[i + 1] = pads[i].btns_d;
                buttons_hold[i + 1] = pads[i].btns_h;
                buttons_released[i + 1] = pads[i].btns_r;

                if (hasNunchuck(&pads[i])) {
                    stickPositions[i + 1][0][0] = pads[i].nunchuck.stick_x;
                    stickPositions[i + 1][0][1] = pads[i].nunchuck.stick_y;
                    stickPositions[i + 1][1][0] = 0;
                    stickPositions[i + 1][1][1] = 0;
                }
            }
            else if (isClassicController(&pads[i])) {
                buttons_pressed[i + 1] = pads[i].classic.btns_d;
                buttons_hold[i + 1] = pads[i].classic.btns_h;
                buttons_released[i + 1] = pads[i].classic.btns_r;

                stickPositions[i + 1][0][0] = pads[i].classic.lstick_x;
                stickPositions[i + 1][0][1] = pads[i].classic.lstick_y;
                stickPositions[i + 1][1][0] = pads[i].classic.rstick_x;
                stickPositions[i + 1][1][1] = pads[i].classic.rstick_y;
            }
            else if (isProController(&pads[i])) {
                buttons_pressed[i + 1] = pads[i].pro.btns_d;
                buttons_hold[i + 1] = pads[i].pro.btns_h;
                buttons_released[i + 1] = pads[i].pro.btns_r;

                stickPositions[i + 1][0][0] = pads[i].pro.lstick_x;
                stickPositions[i + 1][0][1] = pads[i].pro.lstick_y;
                stickPositions[i + 1][1][0] = pads[i].pro.rstick_x;
                stickPositions[i + 1][1][1] = pads[i].pro.rstick_y;
            }
        }
    }
}

bool isWiimote(KPADData *padData){
    return padData->device_type == 0 || padData->device_type == 1 || padData->device_type == 5 || padData->device_type == 6;
}

bool hasNunchuck(KPADData *padData){
    return padData->device_type == 1 || padData->device_type == 6;
}

bool isClassicController(KPADData *padData){
    return padData->device_type == 2 || padData->device_type == 7;
}

bool isProController(KPADData *padData){
    return padData->device_type == 31;
}

bool checkStick(u8 stick, u8 stickDirection, f32 threshold) {
    if (stick == STICK_BOTH) {
        return checkStick(STICK_L, stickDirection, threshold) \
               || checkStick(STICK_R, stickDirection, threshold);
    }

    for (u8 i = 0; i < 4; i++) {
        // Check the sticks on all controllers that aren't Wiimotes
        if (!isWiimote(&pads[i]) || hasNunchuck(&pads[i])) {
            f32 *stickState = stickPositions[i + 1][stick];

            switch (stickDirection) {
                case DIR_DOWN:
                    return stickState[1] < threshold;

                case DIR_UP:
                    return stickState[1] > threshold;

                case DIR_LEFT:
                    return stickState[0] < threshold;

                case DIR_RIGHT:
                    return stickState[0] > threshold;

                case DIR_ANY:
                    return (threshold > 0) ? ((stickState[0] > threshold) || (stickState[1] > threshold)) : \
                    ((stickState[0] < threshold) || (stickState[1] < threshold));
            }
        }
    }

    f32 *stickState = stickPositions[0][stick];
    // Check the sticks on the gamepad
    switch (stickDirection) {
        case DIR_DOWN:
            return stickState[1] < threshold;

        case DIR_UP:
            return stickState[1] > threshold;

        case DIR_LEFT:
            return stickState[0] < threshold;

        case DIR_RIGHT:
            return stickState[0] > threshold;

        case DIR_ANY:
            return (threshold > 0) ? ((stickState[0] > threshold) || (stickState[1] > threshold)) : \
            ((stickState[0] < threshold) || (stickState[1] < threshold));
    }

    return false;
}

int checkButton(int button, int state) {
    uint32_t *stateArray;

    switch(state) {
        case PRESS:
            stateArray = &buttons_pressed;
            break;

        case HOLD:
            stateArray =  &buttons_hold;
            break;

        case RELEASE:
            stateArray = &buttons_released;
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
