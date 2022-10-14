#pragma once

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

#include <algorithm>
#include <array>

#include <coreinit/ios.h>
#include <coreinit/mcp.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <padscore/kpad.h>
#include <sysapp/launch.h>
#include <sysapp/title.h>
#include <vpad/input.h>
#include <whb/proc.h>

#define MIN_MENU_ID 0
#define MAX_MENU_ID 3

enum Menu {
    mainMenu = 0,
    selectTitle = 1,
    selectTask = 2,
    selectOptions = 3
};

Menu operator++(Menu &menu, int) {
    Menu currentMenu = menu;

    if (MAX_MENU_ID < menu + 1) menu = (Menu) MIN_MENU_ID;
    else
        menu = static_cast<Menu>(menu + 1);

    return (currentMenu);
}

Menu operator--(Menu &menu, int) {
    Menu currentMenu = menu;

    if (menu != MIN_MENU_ID)
        menu = static_cast<Menu>(menu - 1);

    return (currentMenu);
}

enum Mode {
    WiiU = 0,
    vWii = 1,
    batchBackup = 2
};

enum Task {
    backup = 0,
    restore = 1,
    wipe = 2,
    importLoadiine = 3,
    exportLoadiine = 4,
    copytoOtherDevice = 5
};