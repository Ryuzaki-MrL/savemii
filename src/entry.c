#include "main.h"
#include "draw.h"
#include "wiiu.h"

int __entry_menu(int argc, char **argv)
{
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

    memoryInitialize();
    VPADInit();
    KPADInit();
    WPADInit();
    drawInit();

    int ret = Menu_Main();

    drawFini();
    memoryRelease();

    return ret;
}
