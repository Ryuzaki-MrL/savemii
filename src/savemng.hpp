#ifndef _SAVEMNG_H_
#define _SAVEMNG_H_

#define __STDC_WANT_LIB_EXT2__ 1

#include "draw.h"
#include "json.h"
#include "log_freetype.h"
#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/thread.h>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <padscore/kpad.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vpad/input.h>

using namespace std;

#define PATH_SIZE 0x400

extern VPADStatus vpad_status;
extern VPADReadError vpad_error;
extern KPADStatus kpad_status;

typedef struct {
    uint32_t highID;
    uint32_t lowID;
    uint16_t listID;
    char shortName[256];
    char longName[512];
    char productCode[5];
    bool saveInit;
    bool isTitleOnUSB;
    bool isTitleDupe;
    uint16_t dupeID;
    uint8_t *iconBuf;
} Title;

typedef struct {
    uint32_t highID;
    uint32_t lowID;
    uint8_t dev;
    bool found;
} Saves;

typedef struct {
    char persistentID[9];
    uint32_t pID;
    char miiName[50];
    uint8_t slot;
} Account;

typedef enum {
    ST_YES_NO = 1,
    ST_CONFIRM_CANCEL = 2,
    ST_MULTILINE = 16,
    ST_WARNING = 32,
    ST_ERROR = 64
} Style;

extern Account *wiiuacc;
extern Account *sdacc;
extern uint8_t wiiuaccn, sdaccn;

void console_print_pos(int x, int y, const char *format, ...);

bool promptConfirm(Style st, const char *question);

void promptError(const char *message, ...);

void getUserID(char *out);

void getAccountsWiiU();

void getAccountsSD(Title *title, uint8_t slot);

bool hasAccountSave(Title *title, bool inSD, bool iine, uint32_t user, uint8_t slot, int version);

int getLoadiineGameSaveDir(char *out, const char *productCode);

int getLoadiineSaveVersionList(int *out, const char *gamePath);

int getLoadiineUserDir(char *out, const char *fullSavePath, const char *userID);

bool isSlotEmpty(uint32_t highID, uint32_t lowID, uint8_t slot);

bool hasCommonSave(Title *title, bool inSD, bool iine, uint8_t slot, int version);

void copySavedata(Title *title, Title *titled, int8_t allusers, int8_t allusers_d, bool common);

void backupAllSave(Title *titles, int count, OSCalendarTime *date);

void backupSavedata(Title *title, uint8_t slot, int8_t allusers, bool common);

void restoreSavedata(Title *title, uint8_t slot, int8_t sdusers, int8_t allusers, bool common);

void wipeSavedata(Title *title, int8_t allusers, bool common);

void importFromLoadiine(Title *title, bool common, int version);

void exportToLoadiine(Title *title, bool common, int version);

void setFSAFD(int fd);

int checkEntry(const char *fPath);

int folderEmpty(const char *fPath);

int32_t loadFile(const char *fPath, uint8_t **buf);

int32_t loadFilePart(const char *fPath, uint32_t start, uint32_t size, uint8_t **buf);

int32_t loadTitleIcon(Title *title);

void show_file_operation(string file_name, string file_src, string file_dest);

void console_print_pos_multiline(int x, int y, char cdiv, const char *format, ...);

void console_print_pos_aligned(int y, uint16_t offset, uint8_t align, const char *format, Args... args);

#endif
