#include "input.h"

VPADStatus vpad_status;
VPADReadError vpad_error;
KPADStatus kpad[4], kpad_status;

void readInput() {
    VPADRead(VPAD_CHAN_0, &vpad_status, 1, &vpad_error);
    if (vpad_error != VPAD_READ_SUCCESS)
        memset(&vpad_status, 0, sizeof(VPADStatus));

    memset(&kpad_status, 0, sizeof(KPADStatus));
    WPADExtensionType controllerType;
    for (int i = 0; i < 4; i++) {
        if (WPADProbe((WPADChan) i, &controllerType) == 0) {
            KPADRead((WPADChan) i, &kpad_status, 1);
            break;
        }
    }
}

bool getInput(ButtonState state, Button button) {
    uint32_t vpadState = 0;
    uint32_t kpadState = 0;

    switch(state) {
        case TRIGGER:
            vpadState = vpad_status.trigger;
            kpadState = kpad_status.trigger;
            break;
        case HOLD:
            vpadState = vpad_status.hold;
            kpadState = kpad_status.hold;
            break;
        case RELEASE:
            vpadState = vpad_status.release;
            kpadState = kpad_status.release;
            break;
    }
    if((vpadState != 0) || (kpadState != 0)) {
        if((button == PAD_BUTTON_ANY) && (vpadState || kpadState))
            return true;
        
        if(vpadState & button)
            return true;
        
        if(kpadState & button)
            return true;
    }
    
    return false;
}