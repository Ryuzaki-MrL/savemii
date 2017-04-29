#include "savemng.h"

#define BUFFER_SIZE 0x80000

void console_print_pos(int x, int y, const char* format, ...) { // Source: ftpiiu

	char* tmp = NULL;

	va_list va;
	va_start(va, format);
	if ((vasprintf(&tmp, format, va) >= 0) && tmp) {

        if (strlen(tmp) > 79) tmp[79] = 0;

        OSScreenPutFontEx(0, x, y, tmp);
        OSScreenPutFontEx(1, x, y, tmp);

	}

	va_end(va);
	if (tmp) free(tmp);

}

bool promptConfirm(const char* question) {
    ucls();
    const char* msg = "(A) Confirm - (B) Cancel";
    int ret = 0;
    while(1) {
        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);
        console_print_pos(25 - (strlen(question)>>1), 8, question);
        console_print_pos(25 - (strlen(msg)>>1), 10, msg);
        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);
        updatePressedButtons();
        if (isPressed(VPAD_BUTTON_A | VPAD_BUTTON_B | VPAD_BUTTON_HOME)) {
            ret = isPressed(VPAD_BUTTON_A);
            break;
        }
    }
    return ret;
}

void promptError(const char* message) {
    ucls();
    while(1) {
        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);
        console_print_pos(25 - (strlen(message)>>1), 9, message);
        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);
        updatePressedButtons();
        if (isPressed(0xFFFF)) break;
    }
}

int DumpFile(char* pPath, const char* output_path) { // Source: ft2sd

    unsigned char* dataBuf = (unsigned char*)memalign(0x40, BUFFER_SIZE);
    if (!dataBuf) {
        promptError("Out of memory.");
        return -1;
    }

    FILE* pReadFile = fopen(pPath, "rb");
    if (!pReadFile) {
        promptError("Failed to open file.");
        free(dataBuf);
        return -2;
    }

    FILE* pWriteFile = fopen(output_path, "wb");
    if (!pWriteFile) {
        promptError("Failed to create file.");
        fclose(pReadFile);
        free(dataBuf);
        return -3;
    }

    u32 rsize;
    while ((rsize = fread(dataBuf, 0x1, BUFFER_SIZE, pReadFile)) > 0) {
        u32 wsize = fwrite(dataBuf, 0x01, rsize, pWriteFile);
        if (wsize!=rsize) {
            promptError("Failed to write to file.");
            fclose(pWriteFile);
            fclose(pReadFile);
            free(dataBuf);
            return -4;
        }
    }

    fclose(pWriteFile);
    fclose(pReadFile);
    free(dataBuf);

    return 0;

}

int DumpDir(char* pPath, const char* target_path) { // Source: ft2sd

    struct dirent* dirent = NULL;
    DIR* dir = NULL;

    dir = opendir(pPath);
    if (dir == NULL) return -1;

    CreateSubfolder(target_path);

    while ((dirent = readdir(dir)) != 0) {

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);

        if (strcmp(dirent->d_name, "..") == 0 || strcmp(dirent->d_name, ".") == 0) continue;

        int len = strlen(pPath);
        snprintf(pPath + len, FS_MAX_FULLPATH_SIZE - len, "/%s", dirent->d_name);

        if (dirent->d_type & DT_DIR) {

            char* targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
            snprintf(targetPath, FS_MAX_FULLPATH_SIZE, "%s/%s", target_path, dirent->d_name);

            CreateSubfolder(targetPath);
            if (DumpDir(pPath, targetPath)!=0) {
                closedir(dir);
                return -2;
            }

            free(targetPath);

        } else {

            char* targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
            snprintf(targetPath, FS_MAX_FULLPATH_SIZE, "%s/%s", target_path, dirent->d_name);

            console_print_pos(0, 0, "Copying file %s", dirent->d_name);
            console_print_pos(0, 1, "From: %s", pPath);
            console_print_pos(0, 2, "To: %s", targetPath);

            if (DumpFile(pPath, targetPath)!=0) {
                closedir(dir);
                return -3;
            }

            free(targetPath);

        }

        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);

        pPath[len] = 0;

    }

    closedir(dir);

    return 0;

}

int DeleteDir(char* pPath) {

    struct dirent* dirent = NULL;
    DIR* dir = NULL;

    dir = opendir(pPath);
    if (dir == NULL) return -1;

    while ((dirent = readdir(dir)) != 0) {

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);

        if (strcmp(dirent->d_name, "..") == 0 || strcmp(dirent->d_name, ".") == 0) continue;

        int len = strlen(pPath);
        snprintf(pPath + len, FS_MAX_FULLPATH_SIZE - len, "/%s", dirent->d_name);

        if (dirent->d_type & DT_DIR) {
            DeleteDir(pPath);
        } else {
            console_print_pos(0, 0, "Deleting file %s", dirent->d_name);
            console_print_pos(0, 1, "From: %s", pPath);
            if (remove(pPath)!=0) promptError("Failed to delete file.");
        }

        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);

        pPath[len] = 0;

    }

    closedir(dir);

    return 0;

}

void getUserID(char* out) { // Source: loadiine_gx2

    /* get persistent ID - thanks to Maschell */
    unsigned int nn_act_handle;
    unsigned long (*GetPersistentIdEx)(unsigned char);
    int (*GetSlotNo)(void);
    void (*nn_Initialize)(void);
    void (*nn_Finalize)(void);
    OSDynLoad_Acquire("nn_act.rpl", &nn_act_handle);
    OSDynLoad_FindExport(nn_act_handle, 0, "GetPersistentIdEx__Q2_2nn3actFUc", &GetPersistentIdEx);
    OSDynLoad_FindExport(nn_act_handle, 0, "GetSlotNo__Q2_2nn3actFv", &GetSlotNo);
    OSDynLoad_FindExport(nn_act_handle, 0, "Initialize__Q2_2nn3actFv", &nn_Initialize);
    OSDynLoad_FindExport(nn_act_handle, 0, "Finalize__Q2_2nn3actFv", &nn_Finalize);

    nn_Initialize(); // To be sure that it is really Initialized

    unsigned char slotno = GetSlotNo();
    unsigned int persistentID = GetPersistentIdEx(slotno);
    nn_Finalize(); //must be called an equal number of times to nn_Initialize

    sprintf(out, "%08X", persistentID);

}

int getLoadiineGameSaveDir(char* out, const char* productCode) {

    struct dirent* dirent = NULL;
    DIR* dir = NULL;

    dir = opendir("sd:/wiiu/saves");
    if (dir == NULL) {
        promptError("Failed to open directory.");
        return -1;
    }

    while ((dirent = readdir(dir)) != 0) {

        if ((dirent->d_type & DT_DIR) && (strstr(dirent->d_name, productCode)!=NULL)) {
            sprintf(out, "sd:/wiiu/saves/%s", dirent->d_name);
            closedir(dir);
            return 0;
        }

    }

    promptError("Loadiine game folder not found.");
    closedir(dir);
    return -2;

}

int getLoadiineSaveVersionList(int* out, const char* gamePath) {

    struct dirent* dirent = NULL;
    DIR* dir = NULL;

    dir = opendir(gamePath);
    if (dir == NULL) {
        promptError("Loadiine game folder not found.");
        return -1;
    }

    int i = 0;
    while ((i < 255) && ((dirent = readdir(dir)) != 0)) {

        if ((dirent->d_type & DT_DIR) && (strchr(dirent->d_name, 'v')!=NULL)) {
            out[++i] = strtol((dirent->d_name)+1, NULL, 10);
        }

    }

    closedir(dir);
    return 0;

}

int getLoadiineUserDir(char* out, const char* fullSavePath, const char* userID) {

    struct dirent* dirent = NULL;
    DIR* dir = NULL;

    dir = opendir(fullSavePath);
    if (dir == NULL) {
        promptError("Failed to open directory.");
        return -1;
    }

    while ((dirent = readdir(dir)) != 0) {

        if ((dirent->d_type & DT_DIR) && (strstr(dirent->d_name, userID))) {
            sprintf(out, "%s/%s", fullSavePath, dirent->d_name);
            closedir(dir);
            return 0;
        }

    }

    sprintf(out, "%s/u", fullSavePath);
    closedir(dir);
    return 0;

}

bool isSlotEmpty(u32 highID, u32 lowID, u8 slot) {
    char path[PATH_SIZE];
    sprintf(path, "sd:/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
    DIR* dir = opendir(path);
    if (dir==NULL) return 1;
    else return closedir(dir);
}

int getEmptySlot(u32 highID, u32 lowID) {
    for (int i = 0; i < 256; i++) {
        if (isSlotEmpty(highID, lowID, i)) return i;
    }
    return -1;
}

void copySavedata(Title* title, Title* titleb, bool allusers, bool common) {
    u32 highID = title->highID, lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    u32 highIDb = titleb->highID, lowIDb = titleb->lowID;
    bool isUSBb = titleb->isTitleOnUSB;

    if (!promptConfirm("Are you sure?")) return;
    int slotb = getEmptySlot(titleb->highID, titleb->lowID);
    if (slotb>=0 && promptConfirm("Backup current savedata first?")) {
	backupSavedata(titleb, slotb, allusers, common);
	promptError("Backup done. Now copying Savedata.");
    }

    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    const char* path = (isUSB ? "storage_usb:/usr/save" : "storage_mlc:/usr/save");
    const char* pathb = (isUSBb ? "storage_usb:/usr/save" : "storage_mlc:/usr/save");
    sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, "user");
    sprintf(dstPath, "%s/%08x/%08x/%s", pathb, highIDb, lowIDb, "user");
    if (!allusers) {
        char usrPath[16];
        getUserID(usrPath);
        u32 srcOffset = strlen(srcPath);
        u32 dstOffset = strlen(dstPath);
        if (common) {
            strcpy(srcPath + srcOffset, "/common");
            strcpy(dstPath + dstOffset, "/common");
            if (DumpDir(srcPath, dstPath)!=0) promptError("Common save not found.");
        }
        sprintf(srcPath + srcOffset, "/%s", usrPath);
        sprintf(dstPath + dstOffset, "/%s", usrPath);
    }
    if (DumpDir(srcPath, dstPath)!=0) promptError("Copy failed.");
    promptError(srcPath);
    promptError(dstPath);
}

void backupSavedata(Title* title, u8 slot, bool allusers, bool common) {

    if (!isSlotEmpty(title->highID, title->lowID, slot) && !promptConfirm("Backup found on this slot. Overwrite it?")) return;
    u32 highID = title->highID, lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB, isWii = (highID==0x00010000);
    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    const char* path = (isWii ? "slccmpt01:/title" : (isUSB ? "storage_usb:/usr/save" : "storage_mlc:/usr/save"));
    sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, isWii ? "data" : "user");
    sprintf(dstPath, "sd:/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
    if (!allusers && !isWii) {
        char usrPath[16];
        getUserID(usrPath);
        u32 srcOffset = strlen(srcPath);
        u32 dstOffset = strlen(dstPath);
        if (common) {
            strcpy(srcPath + srcOffset, "/common");
            strcpy(dstPath + dstOffset, "/common");
            if (DumpDir(srcPath, dstPath)!=0) promptError("Common save not found.");
        }
        sprintf(srcPath + srcOffset, "/%s", usrPath);
        sprintf(dstPath + dstOffset, "/%s", usrPath);
    }
    if (DumpDir(srcPath, dstPath)!=0) promptError("Backup failed. DO NOT restore from this slot.");

}

void restoreSavedata(Title* title, u8 slot, bool allusers, bool common) {

    if (isSlotEmpty(title->highID, title->lowID, slot)) {
        promptError("No backup found on selected slot.");
        return;
    }
    if (!promptConfirm("Are you sure?")) return;
    int slotb = getEmptySlot(title->highID, title->lowID);
    if (slotb>=0 && promptConfirm("Backup current savedata first?")) backupSavedata(title, slotb, allusers, common);
    u32 highID = title->highID, lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB, isWii = (highID==0x00010000);
    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    const char* path = (isWii ? "slccmpt01:/title" : (isUSB ? "storage_usb:/usr/save" : "storage_mlc:/usr/save"));
    sprintf(srcPath, "sd:/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
    sprintf(dstPath, "%s/%08x/%08x/%s", path, highID, lowID, isWii ? "data" : "user");
    if (!allusers && !isWii) {
        char usrPath[16];
        getUserID(usrPath);
        u32 srcOffset = strlen(srcPath);
        u32 dstOffset = strlen(dstPath);
        if (common) {
            strcpy(srcPath + srcOffset, "/common");
            strcpy(dstPath + dstOffset, "/common");
            if (DumpDir(srcPath, dstPath)!=0) promptError("Common save not found.");
        }
        sprintf(srcPath + srcOffset, "/%s", usrPath);
        sprintf(dstPath + dstOffset, "/%s", usrPath);
    }
    if (DumpDir(srcPath, dstPath)!=0) promptError("Restore failed.");

}

void wipeSavedata(Title* title, bool allusers, bool common) {

    if (!promptConfirm("Are you sure?") || !promptConfirm("Hm, are you REALLY sure?")) return;
    int slotb = getEmptySlot(title->highID, title->lowID);
    if (slotb>=0 && promptConfirm("Backup current savedata first?")) backupSavedata(title, slotb, allusers, common);
    u32 highID = title->highID, lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB, isWii = (highID==0x00010000);
    char srcPath[PATH_SIZE];
    const char* path = (isWii ? "slccmpt01:/title" : (isUSB ? "storage_usb:/usr/save" : "storage_mlc:/usr/save"));
    sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, isWii ? "data" : "user");
    if (!allusers && !isWii) {
        u32 offset = strlen(srcPath);
        if (common) {
            strcpy(srcPath + offset, "/common");
            if (DeleteDir(srcPath)!=0) promptError("Common save not found.");
        }
        char usrPath[16];
        getUserID(usrPath);
        sprintf(srcPath + offset, "/%s", usrPath);
    }
    if (DeleteDir(srcPath)!=0) promptError("Failed to delete savefile.");

}

void importFromLoadiine(Title* title, bool common, int version) {

    if (!promptConfirm("Are you sure?")) return;
    int slotb = getEmptySlot(title->highID, title->lowID);
    if (slotb>=0 && promptConfirm("Backup current savedata first?")) backupSavedata(title, slotb, 0, common);
    u32 highID = title->highID, lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    if (getLoadiineGameSaveDir(srcPath, title->productCode)!=0) return;
    if (version) sprintf(srcPath + strlen(srcPath), "/v%i", version);
    char usrPath[16];
    getUserID(usrPath);
    u32 srcOffset = strlen(srcPath);
    getLoadiineUserDir(srcPath, srcPath, usrPath);
    sprintf(dstPath, "storage_%s:/usr/save/%08x/%08x/user", isUSB ? "usb" : "mlc", highID, lowID);
    u32 dstOffset = strlen(dstPath);
    sprintf(dstPath + dstOffset, "/%s", usrPath);
    if (DumpDir(srcPath, dstPath)!=0) promptError("Failed to import savedata from loadiine.");
    strcpy(srcPath + srcOffset, "/c\0");
    strcpy(dstPath + dstOffset, "/common\0");
    if (DumpDir(srcPath, dstPath)!=0) promptError("Common save not found.");

}

void exportToLoadiine(Title* title, bool common, int version) {

    if (!promptConfirm("Are you sure?")) return;
    u32 highID = title->highID, lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    if (getLoadiineGameSaveDir(dstPath, title->productCode)!=0) return;
    if (version) sprintf(dstPath + strlen(dstPath), "/v%u", version);
    char usrPath[16];
    getUserID(usrPath);
    u32 dstOffset = strlen(dstPath);
    getLoadiineUserDir(dstPath, dstPath, usrPath);
    sprintf(srcPath, "storage_%s:/usr/save/%08x/%08x/user", isUSB ? "usb" : "mlc", highID, lowID);
    u32 srcOffset = strlen(srcPath);
    sprintf(srcPath + srcOffset, "/%s", usrPath);
    if (DumpDir(srcPath, dstPath)!=0) promptError("Failed to export savedata to loadiine.");
    strcpy(dstPath + dstOffset, "/c\0");
    strcpy(srcPath + srcOffset, "/common\0");
    if (DumpDir(srcPath, dstPath)!=0) promptError("Common save not found.");

}
