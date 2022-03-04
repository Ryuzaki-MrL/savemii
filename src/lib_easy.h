/*
 * lib_easy.h
 *
 * Provides an easy-to-use library for wii u
 * usefull for test and beginners
 *
 * All the "complex" Wii U stuff is here
 *
*/

#ifndef LIB_EASY_H
#define LIB_EASY_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>

#include <coreinit/screen.h>
#include <coreinit/cache.h>
#include <coreinit/debug.h>
#include <coreinit/ios.h>
#include <coreinit/thread.h>
#include <coreinit/filesystem.h>
#include <coreinit/mcp.h>
//#include <nn/act/client_cpp.h>
#include <coreinit/dynload.h>
#include <vpad/input.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/log.h>
#include <whb/proc.h>
#include <whb/sdcard.h>

//#include "fs/fs_utils.h"
//#include "fs/sd_fat_devoptab.h"
#include "system/memory.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "common/common.h"

extern VPADStatus status;
extern VPADReadError error;

void ucls();
void ScreenInit();
void flipBuffers();
void uInit();
void uDeInit();

#endif /* LIB_EASY_H */