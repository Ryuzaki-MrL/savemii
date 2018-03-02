#ifndef _SAVEMNG_H_
#define _SAVEMNG_H_

#include <sys/dirent.h>
#include <gctypes.h>
#include <fat.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <iosuhax_disc_interface.h>

#include "lib_easy.h"
#include "draw.h"
#include "controllers.h"

#define PATH_SIZE 0x400

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u32 highID;
    u32 lowID;
    u16 listID;
    char shortName[256];
    char longName[512];
    char productCode[5];
    bool saveInit;
    bool isTitleOnUSB;
    bool isTitleDupe;
    u16 dupeID;
    u8* iconBuf;
} Title;

typedef struct {
    u32 highID;
    u32 lowID;
    u8 dev;
    bool found;
} Saves;

typedef struct {
    char persistentID[9];
    u32 pID;
    char miiName[50];
    u8 slot;
} Account;

typedef enum {
    ST_YES_NO = 1,
    ST_CONFIRM_CANCEL = 2,
    ST_MULTILINE = 16,
    ST_WARNING = 32,
    ST_ERROR = 64
} Style;

extern Account* wiiuacc;
extern Account* sdacc;
extern u8 wiiuaccn, sdaccn;

void console_print_pos(int x, int y, const char* format, ...);
bool promptConfirm(Style st, const char* question);
void promptError(const char* message, ...);
void getUserID(char* out);
void getAccountsWiiU();

int getLoadiineGameSaveDir(char* out, const char* productCode);
int getLoadiineSaveVersionList(int* out, const char* gamePath);
int getLoadiineUserDir(char* out, const char* fullSavePath, const char* userID);

u64 getSlotDate(u32 highID, u32 lowID, u8 slot);
bool isSlotEmpty(u32 highID, u32 lowID, u8 slot);
bool hasCommonSave(Title* title, bool inSD, bool iine, u8 slot, int version);

void copySavedata(Title* title, Title* titled, s8 allusers, s8 allusers_d, bool common);
void backupAllSave(Title* titles, int count, OSCalendarTime* date);
void backupSavedata(Title* title, u8 slot, s8 allusers, bool common);
void restoreSavedata(Title* title, u8 slot, s8 sdusers, s8 allusers, bool common);
void wipeSavedata(Title* title, s8 allusers, bool common);
void importFromLoadiine(Title* title, bool common, int version);
void exportToLoadiine(Title* title, bool common, int version);

void setFSAFD(int fd);
int checkEntry(const char * fPath);
int folderEmpty(const char * fPath);
s32 loadFile(const char * fPath, u8 **buf);
s32 loadFilePart(const char * fPath, u32 start, u32 size, u8 **buf);
s32 loadTitleIcon(Title* title);

void show_file_operation(char* file_name, char* file_src, char* file_dest);
void console_print_pos_multiline(int x, int y, char cdiv,const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
