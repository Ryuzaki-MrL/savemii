#include "lib_easy.h"

int vpadError = -1;
VPADData vpad;

s32 padErrors[4];
u32 padTypes[4];
KPADData pads[4];

int screen_buf0_size = 0;
int screen_buf1_size = 0;

void ScreenInit() {
    //Init screen and screen buffers
    OSScreenInit();
    screen_buf0_size = OSScreenGetBufferSizeEx(0);
    screen_buf1_size = OSScreenGetBufferSizeEx(1);
    screenBuffer = MEM1_alloc(screen_buf0_size + screen_buf1_size, 0x100);
    OSScreenSetBufferEx(0, screenBuffer);
    OSScreenSetBufferEx(1, (screenBuffer + screen_buf0_size));
    OSScreenEnableEx(0, 1);
    OSScreenEnableEx(1, 1);
    clearBuffers(); //Clear screens
    initDraw(screenBuffer, screen_buf0_size, screen_buf1_size);
}

void pingControllers() {
    for (int i = 0; i < 4; i++) {
        padErrors[i] = WPADProbe(i, &padTypes[i]);
    }
}

void updatePressedButtons() {
	VPADRead(0, &vpad, 1, &vpadError);
	buttons_pressed[0] = vpad.btns_d;

    pingControllers();
    for (int i = 0; i < 4; i++) {
        if (padErrors[i] == 0) {
            KPADRead(i, &pads[i], 1);
            if (pads[i].device_type < 2) {
                buttons_pressed[i + 1] = pads[i].btns_d;
            } else {
                buttons_pressed[i + 1] = pads[i].classic.btns_d;
            }
        }
    }
}

void updateHeldButtons() {
	VPADRead(0, &vpad, 1, &vpadError);
	buttons_hold[0] = vpad.btns_h;

    pingControllers();
    for (int i = 0; i < 4; i++) {
        if (padErrors[i] == 0) {
            KPADRead(i, &pads[i], 1);
            if (pads[i].device_type < 2) {
                buttons_hold[i + 1] = pads[i].btns_h;
            } else {
                buttons_hold[i + 1] = pads[i].classic.btns_h;
            }
        }
    }
}

void updateReleasedButtons() {
	VPADRead(0, &vpad, 1, &vpadError);
	buttons_released[0] = vpad.btns_r;

    pingControllers();
    for (int i = 0; i < 4; i++) {
        if (padErrors[i] == 0) {
            KPADRead(i, &pads[i], 1);
            if (pads[i].device_type < 2) {
                buttons_released[i + 1] = pads[i].btns_r;
            } else {
                buttons_released[i + 1] = pads[i].classic.btns_r;
            }
        }
    }
}

bool stickPos(u8 stick, f32 value) {
    switch(stick) {
        case 0 :
            return (value > 0) ? (vpad.lstick.x > value): (vpad.lstick.x < value);
        case 1 :
            return (value > 0) ? (vpad.lstick.y > value): (vpad.lstick.y < value);
        case 2 :
            return (value > 0) ? (vpad.rstick.x > value): (vpad.rstick.x < value);
        case 3 :
            return (value > 0) ? (vpad.rstick.y > value): (vpad.rstick.y < value);
        case 4 :
            return ((vpad.lstick.x > value) || (vpad.lstick.x < -value)) || \
                   ((vpad.lstick.y > value) || (vpad.lstick.y < -value)) || \
                   ((vpad.rstick.x > value) || (vpad.rstick.x < -value)) || \
                   ((vpad.rstick.y > value) || (vpad.rstick.y < -value));

        default :
            return 0;
    }
}

bool isWiimote(KPADData *padData){
    return padData->device_type == 0 || padData->device_type == 1 || padData->device_type == 5 || padData->device_type == 6;
}

bool isClassicController(KPADData *padData){
    return padData->device_type == 2 || padData->device_type == 7;
}

bool isProController(KPADData *padData){
    return padData->device_type == 31;
}

int isPressed(int button) {
    //Check for any button at all
    if (button == PAD_BUTTON_ANY) {
        for (int i = 0; i < 5; i++) {
            if (buttons_pressed[i] > 0) return 1;
        }
    }
	//VPad buttons
    switch (button) {
        case PAD_BUTTON_A:
            if (buttons_pressed[0] & VPAD_BUTTON_A) return 1;
            break;

        case PAD_BUTTON_B:
            if (buttons_pressed[0] & VPAD_BUTTON_B) return 1;
            break;

        case PAD_BUTTON_X:
            if (buttons_pressed[0] & VPAD_BUTTON_X) return 1;
            break;

        case PAD_BUTTON_Y:
            if (buttons_pressed[0] & VPAD_BUTTON_Y) return 1;
            break;

        case PAD_BUTTON_UP:
            if (buttons_pressed[0] & VPAD_BUTTON_UP) return 1;
            break;

        case PAD_BUTTON_DOWN:
            if (buttons_pressed[0] & VPAD_BUTTON_DOWN) return 1;
            break;

        case PAD_BUTTON_LEFT:
            if (buttons_pressed[0] & VPAD_BUTTON_LEFT) return 1;
            break;

        case PAD_BUTTON_RIGHT:
            if (buttons_pressed[0] & VPAD_BUTTON_RIGHT) return 1;
            break;

        case PAD_BUTTON_L:
            if (buttons_pressed[0] & VPAD_BUTTON_L) return 1;
            break;

        case PAD_BUTTON_R:
            if (buttons_pressed[0] & VPAD_BUTTON_R) return 1;
            break;

        case PAD_BUTTON_ZL:
            if (buttons_pressed[0] & VPAD_BUTTON_ZL) return 1;
            break;

        case PAD_BUTTON_ZR:
            if (buttons_pressed[0] & VPAD_BUTTON_ZR) return 1;
            break;

        case PAD_BUTTON_PLUS:
            if (buttons_pressed[0] & VPAD_BUTTON_PLUS) return 1;
            break;

        case PAD_BUTTON_MINUS:
            if (buttons_pressed[0] & VPAD_BUTTON_MINUS) return 1;
            break;

        case PAD_BUTTON_HOME:
            if (buttons_pressed[0] & VPAD_BUTTON_HOME) return 1;
            break;

        case PAD_BUTTON_SYNC:
            if (buttons_pressed[0] & VPAD_BUTTON_SYNC) return 1;
            break;

        case PAD_BUTTON_STICK_L:
            if (buttons_pressed[0] & VPAD_BUTTON_L) return 1;
            break;

        case PAD_BUTTON_STICK_R:
            if (buttons_pressed[0] & VPAD_BUTTON_STICK_R) return 1;
            break;

        case PAD_BUTTON_TV:
            if (buttons_pressed[0] & VPAD_BUTTON_TV) return 1;
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
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_1) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_2) return 1;
                        break;

                    case PAD_BUTTON_1:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_1) return 1;
                        break;

                    case PAD_BUTTON_2:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_2) return 1;
                        break;

                    case PAD_BUTTON_Z:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_Z) return 1;
                        break;

                    case PAD_BUTTON_C:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_C) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (buttons_pressed[i + 1] & WPAD_BUTTON_HOME) return 1;
                        break;
                }
            }
            else if (isClassicController(&pads[i])) {
                switch (button) {
                    case PAD_BUTTON_UP:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_X:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_X) return 1;
                        break;

                    case PAD_BUTTON_Y:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_Y) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_L) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_R) return 1;
                        break;

                    case PAD_BUTTON_ZL:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_ZL) return 1;
                        break;

                    case PAD_BUTTON_ZR:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_ZR) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (buttons_pressed[i + 1] & WPAD_CLASSIC_BUTTON_HOME) return 1;
                        break;
                }
            }
            else if (isProController(&pads[i])) {
                switch (button) {
                    case PAD_BUTTON_UP:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_X:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_X) return 1;
                        break;

                    case PAD_BUTTON_Y:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_Y) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (buttons_pressed[i + 1] & WPAD_PRO_TRIGGER_L) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (buttons_pressed[i + 1] & WPAD_PRO_TRIGGER_R) return 1;
                        break;

                    case PAD_BUTTON_ZL:
                        if (buttons_pressed[i + 1] & WPAD_PRO_TRIGGER_ZL) return 1;
                        break;

                    case PAD_BUTTON_ZR:
                        if (buttons_pressed[i + 1] & WPAD_PRO_TRIGGER_ZR) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_HOME) return 1;
                        break;

                    case PAD_BUTTON_STICK_L:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_STICK_L) return 1;
                        break;

                    case PAD_BUTTON_STICK_R:
                        if (buttons_pressed[i + 1] & WPAD_PRO_BUTTON_STICK_R) return 1;
                        break;
                }
            }
        }
        else {
            log_print("No padscore controllers found");
        }
    }

    return 0;
}

int isHeld(int button) {
    //Check for any button at all
    if (button == PAD_BUTTON_ANY) {
        for (int i = 0; i < 5; i++) {
            if (buttons_hold[i] > 0) return 1;
        }
    }
    //VPad buttons
    switch (button) {
        case PAD_BUTTON_A:
            if (buttons_hold[0] & VPAD_BUTTON_A) return 1;
            break;

        case PAD_BUTTON_B:
            if (buttons_hold[0] & VPAD_BUTTON_B) return 1;
            break;

        case PAD_BUTTON_X:
            if (buttons_hold[0] & VPAD_BUTTON_X) return 1;
            break;

        case PAD_BUTTON_Y:
            if (buttons_hold[0] & VPAD_BUTTON_Y) return 1;
            break;

        case PAD_BUTTON_UP:
            if (buttons_hold[0] & VPAD_BUTTON_UP) return 1;
            break;

        case PAD_BUTTON_DOWN:
            if (buttons_hold[0] & VPAD_BUTTON_DOWN) return 1;
            break;

        case PAD_BUTTON_LEFT:
            if (buttons_hold[0] & VPAD_BUTTON_LEFT) return 1;
            break;

        case PAD_BUTTON_RIGHT:
            if (buttons_hold[0] & VPAD_BUTTON_RIGHT) return 1;
            break;

        case PAD_BUTTON_L:
            if (buttons_hold[0] & VPAD_BUTTON_L) return 1;
            break;

        case PAD_BUTTON_R:
            if (buttons_hold[0] & VPAD_BUTTON_R) return 1;
            break;

        case PAD_BUTTON_ZL:
            if (buttons_hold[0] & VPAD_BUTTON_ZL) return 1;
            break;

        case PAD_BUTTON_ZR:
            if (buttons_hold[0] & VPAD_BUTTON_ZR) return 1;
            break;

        case PAD_BUTTON_PLUS:
            if (buttons_hold[0] & VPAD_BUTTON_PLUS) return 1;
            break;

        case PAD_BUTTON_MINUS:
            if (buttons_hold[0] & VPAD_BUTTON_MINUS) return 1;
            break;

        case PAD_BUTTON_HOME:
            if (buttons_hold[0] & VPAD_BUTTON_HOME) return 1;
            break;

        case PAD_BUTTON_SYNC:
            if (buttons_hold[0] & VPAD_BUTTON_SYNC) return 1;
            break;

        case PAD_BUTTON_STICK_L:
            if (buttons_hold[0] & VPAD_BUTTON_L) return 1;
            break;

        case PAD_BUTTON_STICK_R:
            if (buttons_hold[0] & VPAD_BUTTON_STICK_R) return 1;
            break;

        case PAD_BUTTON_TV:
            if (buttons_hold[0] & VPAD_BUTTON_TV) return 1;
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
                        if (buttons_hold[i + 1] & WPAD_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_1) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_2) return 1;
                        break;

                    case PAD_BUTTON_1:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_1) return 1;
                        break;

                    case PAD_BUTTON_2:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_2) return 1;
                        break;

                    case PAD_BUTTON_Z:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_Z) return 1;
                        break;

                    case PAD_BUTTON_C:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_C) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (buttons_hold[i + 1] & WPAD_BUTTON_HOME) return 1;
                        break;
                }
            }
            else if (isClassicController(&pads[i])) {
                switch (button) {
                    case PAD_BUTTON_UP:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_X:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_X) return 1;
                        break;

                    case PAD_BUTTON_Y:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_Y) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_L) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_R) return 1;
                        break;

                    case PAD_BUTTON_ZL:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_ZL) return 1;
                        break;

                    case PAD_BUTTON_ZR:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_ZR) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (buttons_hold[i + 1] & WPAD_CLASSIC_BUTTON_HOME) return 1;
                        break;
                }
            }
            else if (isProController(&pads[i])) {
                switch (button) {
                    case PAD_BUTTON_UP:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_X:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_X) return 1;
                        break;

                    case PAD_BUTTON_Y:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_Y) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (buttons_hold[i + 1] & WPAD_PRO_TRIGGER_L) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (buttons_hold[i + 1] & WPAD_PRO_TRIGGER_R) return 1;
                        break;

                    case PAD_BUTTON_ZL:
                        if (buttons_hold[i + 1] & WPAD_PRO_TRIGGER_ZL) return 1;
                        break;

                    case PAD_BUTTON_ZR:
                        if (buttons_hold[i + 1] & WPAD_PRO_TRIGGER_ZR) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_HOME) return 1;
                        break;

                    case PAD_BUTTON_STICK_L:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_STICK_L) return 1;
                        break;

                    case PAD_BUTTON_STICK_R:
                        if (buttons_hold[i + 1] & WPAD_PRO_BUTTON_STICK_R) return 1;
                        break;
                }
            }
        }
    }

    return 0;
}

int isReleased(int button) {
    //Check for any button at all
    if (button == PAD_BUTTON_ANY) {
        for (int i = 0; i < 5; i++) {
            if (buttons_released[i] > 0) return 1;
        }
    }
    //VPad buttons
    switch (button) {
        case PAD_BUTTON_A:
            if (buttons_released[0] & VPAD_BUTTON_A) return 1;
            break;

        case PAD_BUTTON_B:
            if (buttons_released[0] & VPAD_BUTTON_B) return 1;
            break;

        case PAD_BUTTON_X:
            if (buttons_released[0] & VPAD_BUTTON_X) return 1;
            break;

        case PAD_BUTTON_Y:
            if (buttons_released[0] & VPAD_BUTTON_Y) return 1;
            break;

        case PAD_BUTTON_UP:
            if (buttons_released[0] & VPAD_BUTTON_UP) return 1;
            break;

        case PAD_BUTTON_DOWN:
            if (buttons_released[0] & VPAD_BUTTON_DOWN) return 1;
            break;

        case PAD_BUTTON_LEFT:
            if (buttons_released[0] & VPAD_BUTTON_LEFT) return 1;
            break;

        case PAD_BUTTON_RIGHT:
            if (buttons_released[0] & VPAD_BUTTON_RIGHT) return 1;
            break;

        case PAD_BUTTON_L:
            if (buttons_released[0] & VPAD_BUTTON_L) return 1;
            break;

        case PAD_BUTTON_R:
            if (buttons_released[0] & VPAD_BUTTON_R) return 1;
            break;

        case PAD_BUTTON_ZL:
            if (buttons_released[0] & VPAD_BUTTON_ZL) return 1;
            break;

        case PAD_BUTTON_ZR:
            if (buttons_released[0] & VPAD_BUTTON_ZR) return 1;
            break;

        case PAD_BUTTON_PLUS:
            if (buttons_released[0] & VPAD_BUTTON_PLUS) return 1;
            break;

        case PAD_BUTTON_MINUS:
            if (buttons_released[0] & VPAD_BUTTON_MINUS) return 1;
            break;

        case PAD_BUTTON_HOME:
            if (buttons_released[0] & VPAD_BUTTON_HOME) return 1;
            break;

        case PAD_BUTTON_SYNC:
            if (buttons_released[0] & VPAD_BUTTON_SYNC) return 1;
            break;

        case PAD_BUTTON_STICK_L:
            if (buttons_released[0] & VPAD_BUTTON_L) return 1;
            break;

        case PAD_BUTTON_STICK_R:
            if (buttons_released[0] & VPAD_BUTTON_STICK_R) return 1;
            break;

        case PAD_BUTTON_TV:
            if (buttons_released[0] & VPAD_BUTTON_TV) return 1;
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
                        if (buttons_released[i + 1] & WPAD_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (buttons_released[i + 1] & WPAD_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (buttons_released[i + 1] & WPAD_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (buttons_released[i + 1] & WPAD_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (buttons_released[i + 1] & WPAD_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (buttons_released[i + 1] & WPAD_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (buttons_released[i + 1] & WPAD_BUTTON_1) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (buttons_released[i + 1] & WPAD_BUTTON_2) return 1;
                        break;

                    case PAD_BUTTON_1:
                        if (buttons_released[i + 1] & WPAD_BUTTON_1) return 1;
                        break;

                    case PAD_BUTTON_2:
                        if (buttons_released[i + 1] & WPAD_BUTTON_2) return 1;
                        break;

                    case PAD_BUTTON_Z:
                        if (buttons_released[i + 1] & WPAD_BUTTON_Z) return 1;
                        break;

                    case PAD_BUTTON_C:
                        if (buttons_released[i + 1] & WPAD_BUTTON_C) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (buttons_released[i + 1] & WPAD_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (buttons_released[i + 1] & WPAD_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (buttons_released[i + 1] & WPAD_BUTTON_HOME) return 1;
                        break;
                }
            }
            else if (isClassicController(&pads[i])) {
                switch (button) {
                    case PAD_BUTTON_UP:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_X:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_X) return 1;
                        break;

                    case PAD_BUTTON_Y:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_Y) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_L) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_R) return 1;
                        break;

                    case PAD_BUTTON_ZL:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_ZL) return 1;
                        break;

                    case PAD_BUTTON_ZR:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_ZR) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (buttons_released[i + 1] & WPAD_CLASSIC_BUTTON_HOME) return 1;
                        break;
                }
            }
            else if (isProController(&pads[i])) {
                switch (button) {
                    case PAD_BUTTON_UP:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_UP) return 1;
                        break;

                    case PAD_BUTTON_DOWN:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_DOWN) return 1;
                        break;

                    case PAD_BUTTON_LEFT:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_LEFT) return 1;
                        break;

                    case PAD_BUTTON_RIGHT:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_RIGHT) return 1;
                        break;

                    case PAD_BUTTON_A:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_A) return 1;
                        break;

                    case PAD_BUTTON_B:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_B) return 1;
                        break;

                    case PAD_BUTTON_X:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_X) return 1;
                        break;

                    case PAD_BUTTON_Y:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_Y) return 1;
                        break;

                    case PAD_BUTTON_L:
                        if (buttons_released[i + 1] & WPAD_PRO_TRIGGER_L) return 1;
                        break;

                    case PAD_BUTTON_R:
                        if (buttons_released[i + 1] & WPAD_PRO_TRIGGER_R) return 1;
                        break;

                    case PAD_BUTTON_ZL:
                        if (buttons_released[i + 1] & WPAD_PRO_TRIGGER_ZL) return 1;
                        break;

                    case PAD_BUTTON_ZR:
                        if (buttons_released[i + 1] & WPAD_PRO_TRIGGER_ZR) return 1;
                        break;

                    case PAD_BUTTON_PLUS:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_PLUS) return 1;
                        break;

                    case PAD_BUTTON_MINUS:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_MINUS) return 1;
                        break;

                    case PAD_BUTTON_HOME:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_HOME) return 1;
                        break;

                    case PAD_BUTTON_STICK_L:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_STICK_L) return 1;
                        break;

                    case PAD_BUTTON_STICK_R:
                        if (buttons_released[i + 1] & WPAD_PRO_BUTTON_STICK_R) return 1;
                        break;
                }
            }
        }
    }

    return 0;
}

void uInit() {
    //--Initialize every function pointer-- (byebye FindExport :D)
    InitOSFunctionPointers();
    InitSocketFunctionPointers();
    InitACPFunctionPointers();
    InitAocFunctionPointers();
    InitAXFunctionPointers();
    InitCurlFunctionPointers();
    InitFSFunctionPointers();
    InitGX2FunctionPointers();
    InitPadScoreFunctionPointers();
    InitSysFunctionPointers();
    InitSysHIDFunctionPointers();
    InitVPadFunctionPointers();

    memoryInitialize();				//You probably shouldn't care about this for now :P
    VPADInit();						//Init GamePad input library (needed for getting gamepad input)
    KPADInit();                     //Init controller input library for other wireless inputs
    WPADInit();
    ScreenInit();					//Init OSScreen (all the complex stuff is in easyfunctions.h :P )
}

void uDeInit() {
    MEM1_free(screenBuffer);
    screenBuffer = NULL;
    memoryRelease();
}
