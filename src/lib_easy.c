#include "lib_easy.h"

int vpadError = -1;
VPADData vpad;

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

void updatePressedButtons() {
	VPADRead(0, &vpad, 1, &vpadError);
	buttons_pressed = vpad.btns_d;
}

void updateHeldButtons() {
	VPADRead(0, &vpad, 1, &vpadError);
	buttons_hold = vpad.btns_h;
}

void updateReleasedButtons() {
	VPADRead(0, &vpad, 1, &vpadError);
	buttons_released = vpad.btns_r;
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

int isPressed(int button) {
	return (buttons_pressed&button);
}

int isHeld(int button) {
	return (buttons_hold&button);
}

int isReleased(int button) {
	return (buttons_released&button);
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
    ScreenInit();					//Init OSScreen (all the complex stuff is in easyfunctions.h :P )
}

void uDeInit() {
    MEM1_free(screenBuffer);
    screenBuffer = NULL;
    memoryRelease();
}
