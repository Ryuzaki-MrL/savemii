#define __STDC_WANT_LIB_EXT2__ 1
#define _GNU_SOURCE
#include "savemng.h"

#define BUFFER_SIZE 				0x8020//0x2820
#define BUFFER_SIZE_STEPS           0x20


int fsaFd = -1;
char * p1;


void setFSAFD(int fd) {
	fsaFd = fd;
}

void replace_str(char *str, char *orig, char *rep, int start, char *out) {
  static char temp[PATH_SIZE];
  static char buffer[PATH_SIZE];
  char *p;

  strcpy(temp, str + start);

  if (!(p = strstr(temp, orig)))  // Is 'orig' even in 'temp'?
    return temp;

  strncpy(buffer, temp, p-temp); // Copy characters from 'temp' start to 'orig' str
  buffer[p-temp] = '\0';

  sprintf(buffer + (p - temp), "%s%s", rep, p + strlen(orig));
  sprintf(out, "%s", buffer);
}

void convert_to_string(uint16_t *wide, char *buf) {
	// convert wide-char to normal char
	while (*buf++ = (char)*wide++);
}

int16_t FSAR(int result) {
	return (int16_t) (result & 0xFFFF);
}

int checkEntry(const char * fPath) {
	fileStat_s fStat;
	int ret = FSAR(IOSUHAX_FSA_GetStat(fsaFd, fPath, &fStat));

	if (ret == FS_STATUS_NOT_FOUND) return 0;
	else if (ret < 0) return -1;

	if (fStat.flag & DIR_ENTRY_IS_DIRECTORY) return 2;
	else return 1;
}

int createFolder(const char * fPath) { //Adapted from mkdir_p made by JonathonReinhart
    const size_t len = strlen(fPath);
    char _path[FS_MAX_FULLPATH_SIZE];
    char *p;
	int ret, found = 0;

    if (len > sizeof(_path)-1) {
        return -1;
    }
    strcpy(_path, fPath);

    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
			found++;
			if (found > 2) {
	            *p = '\0';
				if (checkEntry(_path) == 0) {
					if ((ret = FSAR(IOSUHAX_FSA_MakeDir(fsaFd, _path, 0x666))) < 0) return -1;
				}
	            *p = '/';
			}
        }
    }

	if (checkEntry(_path) == 0) {
    	if ((ret = FSAR(IOSUHAX_FSA_MakeDir(fsaFd, _path, 0x666))) < 0) return -1;
	}

    return 0;
}

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

void console_print_pos_va(int x, int y, const char* format, va_list va) { // Source: ftpiiu
	char* tmp = NULL;

	if ((vasprintf(&tmp, format, va) >= 0) && tmp) {
        if (strlen(tmp) > 79) tmp[79] = 0;
        OSScreenPutFontEx(0, x, y, tmp);
        OSScreenPutFontEx(1, x, y, tmp);
	}
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
        VPADRead(VPAD_CHAN_0, &status, 1, &error);
        if (status.trigger & (VPAD_BUTTON_A | VPAD_BUTTON_B | VPAD_BUTTON_HOME)) {
            ret = VPAD_BUTTON_A;
            break;
        }
    }
    return ret;
}

void promptError(const char* message, ...) {
    ucls();
	va_list va;
	va_start(va, message);
    OSScreenClearBufferEx(0, 0);
    OSScreenClearBufferEx(1, 0);
    console_print_pos_va(25 - (strlen(message)>>1), 9, message, va);
    OSScreenFlipBuffersEx(0);
    OSScreenFlipBuffersEx(1);
	va_end(va);
}

void getAccounts() {
    /* get persistent ID - thanks to Maschell */
    unsigned int nn_act_handle;
    unsigned long (*GetPersistentIdEx)(unsigned char);
	bool (*IsSlotOccupied)(unsigned char);
    int (*GetSlotNo)(void);
	int (*GetNumOfAccounts)(void);
	int (*GetMiiNameEx)(uint16_t*, unsigned char);
    void (*nn_Initialize)(void);
    void (*nn_Finalize)(void);
    OSDynLoad_Acquire("nn_act.rpl", &nn_act_handle);
    OSDynLoad_FindExport(nn_act_handle, 0, "GetPersistentIdEx__Q2_2nn3actFUc", &GetPersistentIdEx);
	OSDynLoad_FindExport(nn_act_handle, 0, "IsSlotOccupied__Q2_2nn3actFUc", &IsSlotOccupied);
    OSDynLoad_FindExport(nn_act_handle, 0, "GetSlotNo__Q2_2nn3actFv", &GetSlotNo);
	OSDynLoad_FindExport(nn_act_handle, 0, "GetNumOfAccounts__Q2_2nn3actFv", &GetNumOfAccounts);
	OSDynLoad_FindExport(nn_act_handle, 0, "GetMiiNameEx__Q2_2nn3actFPwUc", &GetMiiNameEx);
    OSDynLoad_FindExport(nn_act_handle, 0, "Initialize__Q2_2nn3actFv", &nn_Initialize);
    OSDynLoad_FindExport(nn_act_handle, 0, "Finalize__Q2_2nn3actFv", &nn_Finalize);

    nn_Initialize(); // To be sure that it is really Initialized

	int i = 0, acc = GetNumOfAccounts();
	char acclist[13][9], nlist[13][11];
	uint16_t out[11];
	while ((acc > 0) && (i <= 12)) {
		if (IsSlotOccupied(i)) {
			unsigned int persistentID = GetPersistentIdEx(i);
			sprintf(acclist[i], "%08X", persistentID);
			GetMiiNameEx(out, i);
			convert_to_string(out, nlist[i]);
			promptError("%d/%s/%s", i, acclist[i], nlist[i]);
			acc--;
		}
		i++;
	}
    nn_Finalize(); //must be called an equal number of times to nn_Initialize
}

int DumpFile(char* pPath, const char* oPath) {
	int srcFd = -1, destFd = -1;
	int ret = 0;
	int buf_size = BUFFER_SIZE;
 	uint8_t * pBuffer;

	do{
		buf_size -= BUFFER_SIZE_STEPS;
		if (buf_size < 0) {
			promptError("Error allocating Buffer.");
			return;
		}
		pBuffer = (uint8_t *)memalign(0x40, buf_size);
		if (pBuffer) memset(pBuffer, 0x00, buf_size);
	}while(!pBuffer);

	ret = IOSUHAX_FSA_OpenFile(fsaFd, pPath, "rb", &srcFd);
	if (ret >= 0) {
		fileStat_s fStat;
		IOSUHAX_FSA_StatFile(fsaFd, srcFd, &fStat);
		if ((ret = IOSUHAX_FSA_OpenFile(fsaFd, oPath, "wb", &destFd)) >= 0) {
			int result, sizew = 0, sizef = fStat.size;
			int fwrite = 0;
			u32 passedMs = 1;
    		u64 startTime = OSGetTime();

			while ((result = IOSUHAX_FSA_ReadFile(fsaFd, pBuffer, 0x01, buf_size, srcFd, 0)) > 0) {
				if ((fwrite = IOSUHAX_FSA_WriteFile(fsaFd, pBuffer, 0x01, result, destFd, 0)) < 0) {
					promptError("Write %d,%s", fwrite, oPath);
					IOSUHAX_FSA_CloseFile(fsaFd, destFd);
					IOSUHAX_FSA_CloseFile(fsaFd, srcFd);
					free(pBuffer);
					return -1;
				}
				sizew += fwrite;
				passedMs = (OSGetTime() - startTime) * 4000ULL / BUS_SPEED;
        		if(passedMs == 0)
            		passedMs = 1;

				OSScreenClearBufferEx(0, 0);
		        OSScreenClearBufferEx(1, 0);
				console_print_pos(0, 0, "Copying file: %s", p1);
	            console_print_pos(0, 1, "From: %s", pPath);
	            console_print_pos(0, 2, "To: %s", oPath);
				console_print_pos(0, 4, "Bytes Copied: %d of %d (%i kB/s)", sizew, sizef,  (u32)(((u64)sizew * 1000) / ((u64)1024 * passedMs)));
				OSScreenFlipBuffersEx(0);
		        OSScreenFlipBuffersEx(1);
			}
		} else {
			promptError("Open File W %d,%s", ret, oPath);
			IOSUHAX_FSA_CloseFile(fsaFd, srcFd);
			free(pBuffer);
			return -1;
		}
		IOSUHAX_FSA_CloseFile(fsaFd, destFd);
		IOSUHAX_FSA_CloseFile(fsaFd, srcFd);
		IOSUHAX_FSA_ChangeMode(fsaFd, oPath, 0x666);
		free(pBuffer);
	} else {
		promptError("Open File R %d,%s", ret, pPath);
		free(pBuffer);
		return -1;
	}

	return 0;
}

int DumpDir(char* pPath, const char* tPath) { // Source: ft2sd
    int dirH;

	if (IOSUHAX_FSA_OpenDir(fsaFd, pPath, &dirH) < 0) return -1;
	IOSUHAX_FSA_MakeDir(fsaFd, tPath, 0x666);

    while (1) {
		directoryEntry_s data;
		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
		if (ret != 0)
			break;

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);

        if (strcmp(data.name, "..") == 0 || strcmp(data.name, ".") == 0) continue;

        int len = strlen(pPath);
        snprintf(pPath + len, FS_MAX_FULLPATH_SIZE - len, "/%s", data.name);

        if (data.stat.flag & DIR_ENTRY_IS_DIRECTORY) {
            char* targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
            snprintf(targetPath, FS_MAX_FULLPATH_SIZE, "%s/%s", tPath, data.name);

            IOSUHAX_FSA_MakeDir(fsaFd, targetPath, 0x666);
            if (DumpDir(pPath, targetPath) != 0) {
                IOSUHAX_FSA_CloseDir(fsaFd, dirH);
                return -2;
            }

            free(targetPath);
        } else {
            char* targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
            snprintf(targetPath, FS_MAX_FULLPATH_SIZE, "%s/%s", tPath, data.name);

			p1 = data.name;
            console_print_pos(0, 0, "Copying file: %s", data.name);
            console_print_pos(0, 1, "From: %s", pPath);
            console_print_pos(0, 2, "To: %s", targetPath);

            if (DumpFile(pPath, targetPath) != 0) {
                IOSUHAX_FSA_CloseDir(fsaFd, dirH);
                return -3;
            }

            free(targetPath);
        }

        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);

        pPath[len] = 0;

    }

	/*if (strcspn(tPath, "_usb") > 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_usb01");
	} else if (strcspn(tPath, "_mlc") > 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_mlc01");
	}*/

    IOSUHAX_FSA_CloseDir(fsaFd, dirH);

    return 0;

}

int DeleteDir(char* pPath) {
	int dirH;

	if (IOSUHAX_FSA_OpenDir(fsaFd, pPath, &dirH) < 0) return -1;

	while (1) {
		directoryEntry_s data;
		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
		if (ret != 0)
			break;

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);

        if (strcmp(data.name, "..") == 0 || strcmp(data.name, ".") == 0) continue;

        int len = strlen(pPath);
        snprintf(pPath + len, FS_MAX_FULLPATH_SIZE - len, "/%s", data.name);

        if (data.stat.flag & DIR_ENTRY_IS_DIRECTORY) {
			char origPath[PATH_SIZE];
			sprintf(origPath, "%s", pPath);
            DeleteDir(pPath);

			OSScreenClearBufferEx(0, 0);
	        OSScreenClearBufferEx(1, 0);
			console_print_pos(0, 0, "Deleting folder %s", data.name);
            console_print_pos(0, 1, "From: %s", origPath);
            if (IOSUHAX_FSA_Remove(fsaFd, origPath) != 0) promptError("Failed to delete folder.");
        } else {
            console_print_pos(0, 0, "Deleting file %s", data.name);
            console_print_pos(0, 1, "From: %s", pPath);
            if (IOSUHAX_FSA_Remove(fsaFd, pPath) != 0) promptError("Failed to delete file.");
        }

        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);
        pPath[len] = 0;
    }

    IOSUHAX_FSA_CloseDir(fsaFd, dirH);
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
	int dirH;

	if (IOSUHAX_FSA_OpenDir(fsaFd, "/vol/storage_sdcard/wiiu/saves", &dirH) < 0) return -1;

	while (1) {
		directoryEntry_s data;
		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
		if (ret != 0)
			break;

        if ((data.stat.flag & DIR_ENTRY_IS_DIRECTORY) && (strstr(data.name, productCode) != NULL)) {
            sprintf(out, "/vol/storage_sdcard/wiiu/saves/%s", data.name);
            IOSUHAX_FSA_CloseDir(fsaFd, dirH);
            return 0;
        }
    }

    promptError("Loadiine game folder not found.");
    IOSUHAX_FSA_CloseDir(fsaFd, dirH);
    return -2;
}

int getLoadiineSaveVersionList(int* out, const char* gamePath) {
	int dirH;

	if (IOSUHAX_FSA_OpenDir(fsaFd, gamePath, &dirH) < 0) {
		promptError("Loadiine game folder not found.");
		return -1;
	}

    int i = 0;
    while (i < 255) {
		directoryEntry_s data;
		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
		if (ret != 0)
			break;

        if ((data.stat.flag & DIR_ENTRY_IS_DIRECTORY) && (strchr(data.name, 'v') != NULL)) {
            out[++i] = strtol((data.name)+1, NULL, 10);
        }

    }

    IOSUHAX_FSA_CloseDir(fsaFd, dirH);
    return 0;
}

int getLoadiineUserDir(char* out, const char* fullSavePath, const char* userID) {
	int dirH;

	if (IOSUHAX_FSA_OpenDir(fsaFd, fullSavePath, &dirH) < 0) {
        promptError("Failed to open directory.");
        return -1;
    }

    while (1) {
		directoryEntry_s data;
		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
		if (ret != 0)
			break;

        if ((data.stat.flag & DIR_ENTRY_IS_DIRECTORY) && (strstr(data.name, userID))) {
            sprintf(out, "%s/%s", fullSavePath, data.name);
            IOSUHAX_FSA_CloseDir(fsaFd, dirH);
            return 0;
        }
    }

    sprintf(out, "%s/u", fullSavePath);
    IOSUHAX_FSA_CloseDir(fsaFd, dirH);
    return 0;
}

bool isSlotEmpty(u32 highID, u32 lowID, u8 slot) {
    char path[PATH_SIZE];
    sprintf(path, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
	int ret = checkEntry(path);
    if (ret <= 0) return 1;
    else return 0;
}

int getEmptySlot(u32 highID, u32 lowID) {
    for (int i = 0; i < 256; i++) {
        if (isSlotEmpty(highID, lowID, i)) return i;
    }
    return -1;
}

bool hasCommonSave(Title* title, bool inSD, bool iine, u8 slot, int version) {
    u32 highID = title->highID, lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB, isWii = (highID==0x00010000);
	if (isWii) return false;

    char srcPath[PATH_SIZE];
    const char* path = (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save");
	if (!inSD)
    	sprintf(srcPath, "%s/%08x/%08x/%s/common", path, highID, lowID, "user");
	else {
		if (!iine)
    		sprintf(srcPath, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u/common", highID, lowID, slot);
		else {
			if (getLoadiineGameSaveDir(srcPath, title->productCode) != 0) return false;
			if (version) sprintf(srcPath + strlen(srcPath), "/v%u", version);
			char usrPath[16];
			getUserID(usrPath);
			u32 srcOffset = strlen(srcPath);
			getLoadiineUserDir(srcPath, srcPath, usrPath);
			strcpy(srcPath + srcOffset, "/c\0");
		}
	}
	if (checkEntry(srcPath) == 2)
		return true;
	return false;
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
    const char* path = (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save");
    const char* pathb = (isUSBb ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save");
    sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, "user");
    sprintf(dstPath, "%s/%08x/%08x/%s", pathb, highIDb, lowIDb, "user");
	createFolder(dstPath);

    if (!allusers) {
        char usrPath[16];
        getUserID(usrPath);
        u32 srcOffset = strlen(srcPath);
        u32 dstOffset = strlen(dstPath);
        if (common) {
            strcpy(srcPath + srcOffset, "/common");
            strcpy(dstPath + dstOffset, "/common");
            if (DumpDir(srcPath, dstPath) != 0) promptError("Common save not found.");
        }
        sprintf(srcPath + srcOffset, "/%s", usrPath);
        sprintf(dstPath + dstOffset, "/%s", usrPath);
    }
    if (DumpDir(srcPath, dstPath) != 0) promptError("Copy failed.");

	if (strcspn(dstPath, "_usb") > 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_usb01");
	} else if (strcspn(dstPath, "_mlc") > 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_mlc01");
	}
}

void backupSavedata(Title* title, u8 slot, bool allusers, bool common) {

    if (!isSlotEmpty(title->highID, title->lowID, slot) && !promptConfirm("Backup found on this slot. Overwrite it?")) return;
    u32 highID = title->highID, lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB, isWii = (highID==0x00010000);
    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    const char* path = (isWii ? "/vol/storage_slccmpt01/title" : (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save"));
    sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, isWii ? "data" : "user");
	sprintf(dstPath, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
	createFolder(dstPath);

    if (!allusers && !isWii) {
        char usrPath[16];
        getUserID(usrPath);
        u32 srcOffset = strlen(srcPath);
        u32 dstOffset = strlen(dstPath);
        if (common) {
            strcpy(srcPath + srcOffset, "/common");
            strcpy(dstPath + dstOffset, "/common");
            if (DumpDir(srcPath, dstPath) != 0) promptError("Common save not found.");
        }
        sprintf(srcPath + srcOffset, "/%s", usrPath);
        sprintf(dstPath + dstOffset, "/%s", usrPath);
		if (checkEntry(srcPath) == 0) {
			promptError("No save found for this user.");
			return;
		}
    }
    if (DumpDir(srcPath, dstPath) != 0) promptError("Backup failed. DO NOT restore from this slot.");

	if (strcspn(dstPath, "_usb") > 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_usb01");
	} else if (strcspn(dstPath, "_mlc") > 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_mlc01");
	}
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
    const char* path = (isWii ? "/vol/storage_slccmpt01/title" : (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save"));
    sprintf(srcPath, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
    sprintf(dstPath, "%s/%08x/%08x/%s", path, highID, lowID, isWii ? "data" : "user");
	createFolder(dstPath);

    if (!allusers && !isWii) {
        char usrPath[16];
        getUserID(usrPath);
        u32 srcOffset = strlen(srcPath);
        u32 dstOffset = strlen(dstPath);
        if (common) {
            strcpy(srcPath + srcOffset, "/common");
            strcpy(dstPath + dstOffset, "/common");
            if (DumpDir(srcPath, dstPath) != 0) promptError("Common save not found.");
        }
        sprintf(srcPath + srcOffset, "/%s", usrPath);
        sprintf(dstPath + dstOffset, "/%s", usrPath);
    }
    if (DumpDir(srcPath, dstPath) != 0) promptError("Restore failed.");

	if (strcspn(dstPath, "_usb") > 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_usb01");
	} else if (strcspn(dstPath, "_mlc") > 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_mlc01");
	}
}

void wipeSavedata(Title* title, bool allusers, bool common) {
    if (!promptConfirm("Are you sure?") || !promptConfirm("Hm, are you REALLY sure?")) return;
    int slotb = getEmptySlot(title->highID, title->lowID);
    if (slotb>=0 && promptConfirm("Backup current savedata first?")) backupSavedata(title, slotb, allusers, common);
    u32 highID = title->highID, lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB, isWii = (highID==0x00010000);
    char srcPath[PATH_SIZE];
	char origPath[PATH_SIZE];
    const char* path = (isWii ? "/vol/storage_slccmpt01/title" : (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save"));
    sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, isWii ? "data" : "user");
    if (!allusers && !isWii) {
        u32 offset = strlen(srcPath);
        if (common) {
            strcpy(srcPath + offset, "/common");
			sprintf(origPath, "%s", srcPath);
            if (DeleteDir(srcPath)!=0) promptError("Common save not found.");
			if (IOSUHAX_FSA_Remove(fsaFd, origPath) != 0) promptError("Failed to delete common folder.");
        }
        char usrPath[16];
        getUserID(usrPath);
        sprintf(srcPath + offset, "/%s", usrPath);
		sprintf(origPath, "%s", srcPath);
    }
    if (DeleteDir(srcPath)!=0) promptError("Failed to delete savefile.");
	if (!allusers && !isWii) {
		if (IOSUHAX_FSA_Remove(fsaFd, origPath) != 0) promptError("Failed to delete user folder.");
	}
	if (strcspn(srcPath, "_usb") > 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_usb01");
	} else if (strcspn(srcPath, "_mlc") > 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_mlc01");
	}
}

void importFromLoadiine(Title* title, bool common, int version) {

    if (!promptConfirm("Are you sure?")) return;
    int slotb = getEmptySlot(title->highID, title->lowID);
    if (slotb>=0 && promptConfirm("Backup current savedata first?")) backupSavedata(title, slotb, 0, common);
    u32 highID = title->highID, lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    if (getLoadiineGameSaveDir(srcPath, title->productCode) !=0 ) return;
    if (version) sprintf(srcPath + strlen(srcPath), "/v%i", version);
    char usrPath[16];
    getUserID(usrPath);
    u32 srcOffset = strlen(srcPath);
    getLoadiineUserDir(srcPath, srcPath, usrPath);
    sprintf(dstPath, "/vol/storage_%s01/usr/save/%08x/%08x/user", isUSB ? "usb" : "mlc", highID, lowID);
	createFolder(dstPath);
    u32 dstOffset = strlen(dstPath);
    sprintf(dstPath + dstOffset, "/%s", usrPath);
    if (DumpDir(srcPath, dstPath) != 0) promptError("Failed to import savedata from loadiine.");
    strcpy(srcPath + srcOffset, "/c\0");
    strcpy(dstPath + dstOffset, "/common\0");
    if (DumpDir(srcPath, dstPath) != 0) promptError("Common save not found.");

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
    sprintf(srcPath, "/vol/storage_%s01/usr/save/%08x/%08x/user", isUSB ? "usb" : "mlc", highID, lowID);
    u32 srcOffset = strlen(srcPath);
    sprintf(srcPath + srcOffset, "/%s", usrPath);
    if (DumpDir(srcPath, dstPath)!=0) promptError("Failed to export savedata to loadiine.");
    strcpy(dstPath + dstOffset, "/c\0");
    strcpy(srcPath + srcOffset, "/common\0");
    if (DumpDir(srcPath, dstPath)!=0) promptError("Common save not found.");

}
