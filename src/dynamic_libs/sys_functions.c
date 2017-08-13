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

u32 sysapp_handle __attribute__((section(".data"))) = 0;

EXPORT_DECL(s32, _SYSLaunchTitleByPathFromLauncher, const char* path, s32 len, s32 zero);
EXPORT_DECL(s32, SYSRelaunchTitle, s32 argc, char** argv);
EXPORT_DECL(s32, SYSLaunchMenu, void);
EXPORT_DECL(s32, SYSCheckTitleExists, u64 titleId);
EXPORT_DECL(s32, SYSLaunchTitle, u64 titleId);
EXPORT_DECL(s32, SYSLaunchSettings, s32 unk);

void InitAcquireSys(void)
{
    OSDynLoad_Acquire("sysapp.rpl", &sysapp_handle);
}

void InitSysFunctionPointers(void)
{
    u32 *funcPointer = 0;
    InitAcquireSys();

    OS_FIND_EXPORT(sysapp_handle, _SYSLaunchTitleByPathFromLauncher);
    OS_FIND_EXPORT(sysapp_handle, SYSRelaunchTitle);
    OS_FIND_EXPORT(sysapp_handle, SYSLaunchMenu);
    OS_FIND_EXPORT(sysapp_handle, SYSCheckTitleExists);
    OS_FIND_EXPORT(sysapp_handle, SYSLaunchTitle);
    OS_FIND_EXPORT(sysapp_handle, SYSLaunchSettings);
}
