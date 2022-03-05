#include "main.h"
#include "draw.h"
#include "wiiu.h"

int __entry_menu(int argc, char **argv)
{
    VPADInit();
    KPADInit();
    WPADInit();
    drawInit();

    int ret = Menu_Main();

    drawFini();
    memoryRelease();

    return ret;
}
