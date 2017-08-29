/****************************************************************************
 * Copyright (C) 2015
 * by Dimok
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 ***************************************************************************/
#include "os_functions.h"
#include "vpad_functions.h"

u32 vpad_handle __attribute__((section(".data"))) = 0;
u32 vpadbase_handle __attribute__((section(".data"))) = 0;

EXPORT_DECL(void, VPADInit, void);
EXPORT_DECL(s32, VPADRead, s32 chan, VPADData *buffer, u32 buffer_size, s32 *error);
EXPORT_DECL(s32, VPADGetLcdMode, s32 padnum, s32 *lcdmode);
EXPORT_DECL(s32, VPADSetLcdMode, s32 padnum, s32 lcdmode);
EXPORT_DECL(s32, VPADBASEGetMotorOnRemainingCount, s32 padnum);
EXPORT_DECL(s32, VPADBASESetMotorOnRemainingCount, s32 padnum, s32 counter);

void InitAcquireVPad(void)
{
    OSDynLoad_Acquire("vpad.rpl", &vpad_handle);
    OSDynLoad_Acquire("vpadbase.rpl", &vpadbase_handle);
}

void InitVPadFunctionPointers(void)
{
    u32 *funcPointer = 0;

    InitAcquireVPad();

    OS_FIND_EXPORT(vpad_handle, VPADInit);
    OS_FIND_EXPORT(vpad_handle, VPADRead);
    OS_FIND_EXPORT(vpad_handle, VPADGetLcdMode);
    OS_FIND_EXPORT(vpad_handle, VPADSetLcdMode);
    OS_FIND_EXPORT(vpadbase_handle, VPADBASEGetMotorOnRemainingCount);
    OS_FIND_EXPORT(vpadbase_handle, VPADBASESetMotorOnRemainingCount);
}
