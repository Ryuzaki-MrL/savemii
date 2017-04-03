#ifndef _SAVEMNG_H_
#define _SAVEMNG_H_

#include <gctypes.h>
#include <fat.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <iosuhax_disc_interface.h>

#include "lib_easy.h"

#ifdef __cplusplus
extern "C" {
#endif

void console_print_pos(int x, int y, const char *format, ...);

void backupSavedata(u32 highID, u32 lowID, bool isUSB, int slot);
void restoreSavedata(u32 highID, u32 lowID, bool isUSB, int slot);
void wipeSavedata(u32 highID, u32 lowID, bool isUSB);

#ifdef __cplusplus
}
#endif

#endif