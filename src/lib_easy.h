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
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>

#include "dynamic_libs/ax_functions.h"
#include "dynamic_libs/sys_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "dynamic_libs/fs_functions.h"
#include "dynamic_libs/aoc_functions.h"
#include "dynamic_libs/syshid_functions.h"
#include "dynamic_libs/padscore_functions.h"
#include "dynamic_libs/fs_defs.h"
#include "dynamic_libs/vpad_functions.h"
#include "dynamic_libs/gx2_types.h"
#include "dynamic_libs/curl_functions.h"
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/gx2_functions.h"
#include "dynamic_libs/acp_functions.h"
#include "fs/fs_utils.h"
#include "fs/sd_fat_devoptab.h"
#include "system/memory.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "common/common.h"

unsigned char *screenBuffer;
uint32_t buttons_hold; //Held buttons
uint32_t buttons_pressed; //Pressed buttons
uint32_t buttons_released; //Released buttons

void ScreenInit();
void updatePressedButtons();
void updateHeldButtons();
void updateReleasedButtons();
bool stickPos(u8 stick, f32 value);
int isPressed(int button);
int isHeld(int button);
int isReleased(int button);
void uInit();
void uDeInit();

#endif /* LIB_EASY_H */
