#include <string.h>
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/sys_functions.h"
#include "common/common.h"
#include "utils/utils.h"
#include "main.h"
#include "lib_easy.h"

int __entry_menu(int argc, char **argv)
{
    uInit(); //Init all the wii u stuff
    int ret=Menu_Main(); //Jump to our application
    uDeInit();
    return ret;
}
