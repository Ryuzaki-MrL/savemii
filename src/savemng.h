#ifndef _SAVEMNG_H_
#define _SAVEMNG_H_

#include <sys/dirent.h>
#include <gctypes.h>
#include <fat.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <iosuhax_disc_interface.h>

#include "lib_easy.h"

#define PATH_SIZE 0x200

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u32 highID;
    u32 lowID;
    char shortName[256];
    char productCode[32];
    bool isTitleOnUSB;
} Title;

void console_print_pos(int x, int y, const char* format, ...);
bool promptConfirm(const char* question);
void promptError(const char* message);

int getLoadiineGameSaveDir(char* out, const char* productCode);
int getLoadiineSaveVersionList(int* out, const char* gamePath);
int getLoadiineUserDir(char* out, const char* fullSavePath, const char* userID);

bool isSlotEmpty(u32 highID, u32 lowID, u8 slot);

void backupSavedata(Title* title, u8 slot, bool allusers, bool common);
void restoreSavedata(Title* title, u8 slot, bool allusers, bool common);
void wipeSavedata(Title* title, bool allusers, bool common);
void importFromLoadiine(Title* title, bool common, int version);
void exportToLoadiine(Title* title, bool common, int version);

#ifdef __cplusplus
}
#endif

#endif