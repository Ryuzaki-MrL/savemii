#include <nn/act/client_cpp.h>
#include <sys/stat.h>
extern "C" {
	#include "common/fs_defs.h"
	#include "savemng.h"
}
using namespace std;

#define BUFFER_SIZE 				0x8020
#define BUFFER_SIZE_STEPS           0x20

int fsaFd = -1;
char * p1;
Account* wiiuacc;
Account* sdacc;
u8 wiiuaccn = 0, sdaccn = 5;
VPADStatus status;
VPADReadError error;

void setFSAFD(int fd) {
	fsaFd = fd;
}

void show_file_operation(const char* file_name, const char* file_src, const char* file_dest) {
	char dev_s[100], dev_d[100];

	if (strncmp(strchr(file_src, '_'), "_usb", 4) == 0) {
		strcpy(dev_s, " (USB)");
	} else if (strncmp(strchr(file_src, '_'), "_mlc", 4) == 0) {
		strcpy(dev_s, " (NAND WiiU)");
	} else if (strncmp(strchr(file_src, '_'), "_slccmpt", 8) == 0) {
		strcpy(dev_s, " (NAND vWii)");
	} else if (strncmp(strchr(file_src, '_'), "_sdcard", 7) == 0) {
		strcpy(dev_s, " (SD)");
	}

	if (strncmp(strchr(file_dest, '_'), "_usb", 4) == 0) {
		strcpy(dev_d, " (USB)");
	} else if (strncmp(strchr(file_dest, '_'), "_mlc", 4) == 0) {
		strcpy(dev_d, " (NAND WiiU)");
	} else if (strncmp(strchr(file_dest, '_'), "_slccmpt", 8) == 0) {
		strcpy(dev_d, " (NAND vWii)");
	} else if (strncmp(strchr(file_dest, '_'), "_sdcard", 7) == 0) {
		strcpy(dev_d, " (SD)");
	}

	console_print_pos(-2, 0, "Copying file: %s", file_name);
	console_print_pos_multiline(-2, 2, '/', "From%s: \n%s", dev_s, strstr(strstr(strstr(file_src, "/") + 1,"/") + 1,"/"));
	console_print_pos_multiline(-2, 8, '/', "To%s: \n%s", dev_d, strstr(strstr(strstr(file_dest, "/") + 1,"/") + 1,"/"));
}

int FSAR(int result) {
	if ((result & 0xFFFF0000) == 0xFFFC0000)
		return (result & 0xFFFF) | 0xFFFF0000;
	else
		return result;
}

s32 loadFile(const char * fPath, u8 **buf) {
	int srcFd = -1;
	int ret = IOSUHAX_FSA_OpenFile(fsaFd, fPath, "rb", &srcFd);
	if (ret >= 0) {
		fileStat_s fStat;
		IOSUHAX_FSA_StatFile(fsaFd, srcFd, &fStat);
		size_t size = fStat.size;

		*buf = (u8*)malloc(size);
		if (*buf) {
			memset(*buf, 0, size);
			ret = IOSUHAX_FSA_ReadFile(fsaFd, *buf, 0x01, size, srcFd, 0);
		}
		IOSUHAX_FSA_CloseFile(fsaFd, srcFd);
	}
	return ret;
}

s32 loadFilePart(const char * fPath, u32 start, u32 size, u8 **buf) {
	int srcFd = -1;
	int ret = IOSUHAX_FSA_OpenFile(fsaFd, fPath, "rb", &srcFd);
	if (ret >= 0) {
		fileStat_s fStat;
		IOSUHAX_FSA_StatFile(fsaFd, srcFd, &fStat);
		if ((start + size) > fStat.size) {
			IOSUHAX_FSA_CloseFile(fsaFd, srcFd);
			return -43;
		}
		IOSUHAX_FSA_SetFilePos(fsaFd, srcFd, start);

		*buf = (u8*)malloc(size);
		if (*buf) {
			memset(*buf, 0, size);
			ret = IOSUHAX_FSA_ReadFile(fsaFd, *buf, 0x01, size, srcFd, 0);
		}
		IOSUHAX_FSA_CloseFile(fsaFd, srcFd);
	}
	return ret;
}

s32 loadTitleIcon(Title* title) {
	u32 highID = title->highID, lowID = title->lowID;
	bool isUSB = title->isTitleOnUSB, isWii = ((highID & 0xFFFFFFF0) == 0x00010000);
	char path[256];
	memset(path, 0, sizeof(path));

	if (isWii) {
		if (title->saveInit) {
			sprintf(path, "/vol/storage_slccmpt01/title/%08x/%08x/data/banner.bin", highID, lowID);
			return loadFilePart(path, 0xA0, 24576, &title->iconBuf);
		}
	} else {
		if (title->saveInit)
			sprintf(path, "/vol/storage_%s01/usr/save/%08x/%08x/meta/iconTex.tga", isUSB ? "usb" : "mlc", highID, lowID);
		else
			sprintf(path, "/vol/storage_%s01/usr/title/%08x/%08x/meta/iconTex.tga", isUSB ? "usb" : "mlc", highID, lowID);

		return loadFile(path, &title->iconBuf);
	}
	return -23;
}

int checkEntry(const char * fPath) {
	fileStat_s fStat;
	int ret = FSAR(IOSUHAX_FSA_GetStat(fsaFd, fPath, &fStat));

	if (ret == FSA_STATUS_NOT_FOUND) return 0;
	else if (ret < 0) return -1;

	if (fStat.flag & DIR_ENTRY_IS_DIRECTORY) return 2;
	return 1;
}

int folderEmpty(const char * fPath) {
	int dirH;

	if (IOSUHAX_FSA_OpenDir(fsaFd, fPath, &dirH) >= 0) {
		directoryEntry_s data;
		int ret = FSAR(IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data));
		IOSUHAX_FSA_CloseDir(fsaFd, dirH);
		if (ret == FSA_STATUS_END_OF_DIRECTORY)
			return 1;
	} else return -1;
	return 0;
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

void console_print_pos_aligned(int y, u16 offset, u8 align, const char* format, ...) {
	char* tmp = NULL;
	int x = 0;
	va_list va;
	va_start(va, format);
	if ((vasprintf(&tmp, format, va) >= 0) && tmp) {
		if (strlen(tmp) > 79) tmp[79] = 0;
		switch(align) {
			case 0: x = (offset * 12); break;
			case 1: x = (853 - strlen(tmp)) / 2; break;
			case 2: x = 853 - (offset * 12) - strlen(tmp); break;
			default:  x = (853 - strlen(tmp)) / 2; break;
		}
        OSScreenPutFontEx(SCREEN_TV, x, y, tmp);
        OSScreenPutFontEx(SCREEN_DRC, x, y, tmp);
	}
	va_end(va);
	if (tmp) free(tmp);
}

void console_print_pos(int x, int y, const char* format, ...) { // Source: ftpiiu
	char* tmp = NULL;

	va_list va;
	va_start(va, format);
	if ((vasprintf(&tmp, format, va) >= 0) && tmp) {
        if (strlen(tmp) > 79) tmp[79] = 0;
        OSScreenPutFontEx(SCREEN_TV, x, y, tmp);
        OSScreenPutFontEx(SCREEN_DRC, x, y, tmp);
	}

	va_end(va);
	if (tmp) free(tmp);
}

void console_print_pos_multiline(int x, int y, char cdiv, const char* format, ...) { // Source: ftpiiu
	char* tmp = NULL;
	int len = (66 - x);

	va_list va;
	va_start(va, format);
	if ((vasprintf(&tmp, format, va) >= 0) && tmp) {
		if ((strlen(tmp) / 12) > (unsigned)len) {
			char* p = tmp;
			if (strrchr(p, '\n') != NULL) p = strrchr(p, '\n') + 1;
			while((strlen(p) / 12) > (unsigned)len) {
				char* q = p;
				int l1 = strlen(q);
				for(int i = l1; i > 0; i--) {
					char o = q[l1];
					q[l1] = '\0';
					if ((strlen(p) / 12) <= (unsigned)len) {
						if (strrchr(p, cdiv) != NULL) p = strrchr(p, cdiv) + 1;
						else p = q + l1;
						q[l1] = o;
						break;
					}
					q[l1] = o;
					l1--;
				}
				char buf[256];
				memset(buf, 0, sizeof(buf));
				strcpy(buf, p);
				sprintf(p, "\n%s", buf);
				p++;
				len = 69;
			}
		}
		OSScreenPutFontEx(SCREEN_TV, x, y, tmp);
		OSScreenPutFontEx(SCREEN_DRC, x, y, tmp);
	}
	va_end(va);
	if (tmp) free(tmp);
}

void console_print_pos_va(int x, int y, const char* format, va_list va) { // Source: ftpiiu
	char* tmp = NULL;

	if ((vasprintf(&tmp, format, va) >= 0) && tmp) {
		OSScreenPutFontEx(SCREEN_TV, x, y, tmp);
		OSScreenPutFontEx(SCREEN_DRC, x, y, tmp);
	}
	if (tmp) free(tmp);
}

bool promptConfirm(Style st, const char* question) {
    clearBuffers();
	const char* msg1 = "(A) Yes - (B) No";
	const char* msg2 = "(A) Confirm - (B) Cancel";
	const char* msg;
	switch(st & 0x0F) {
		case ST_YES_NO: msg = msg1; break;
		case ST_CONFIRM_CANCEL: msg = msg2; break;
		default: msg = msg2;
	}
	if (st & ST_WARNING) {
		OSScreenClearBufferEx(SCREEN_TV, 0x7F7F0000);
	    OSScreenClearBufferEx(SCREEN_DRC, 0x7F7F0000);
	} else if (st & ST_ERROR) {
		OSScreenClearBufferEx(SCREEN_TV, 0x7F000000);
	    OSScreenClearBufferEx(SCREEN_DRC, 0x7F000000);
	} else {
		OSScreenClearBufferEx(SCREEN_TV, 0x007F0000);
	    OSScreenClearBufferEx(SCREEN_DRC, 0x007F0000);
	}
	if (st & ST_MULTILINE) {

	} else {
    	console_print_pos(31 - (strlen(question) / 24), 7, question);
    	console_print_pos(31 - (strlen(msg) / 24), 9, msg);
	}
    flipBuffers();
	int ret = 0;
    while(1) {	
        VPADRead(VPAD_CHAN_0, &status, 1, &error);
        if (status.release & (VPAD_BUTTON_A | VPAD_BUTTON_B | VPAD_BUTTON_HOME)) {
            ret = VPAD_BUTTON_A;
            break;
        }
    }
    return ret;
}

void promptError(const char* message, ...) {
    clearBuffers();
	va_list va;
	va_start(va, message);
    OSScreenClearBufferEx(SCREEN_TV, 0x7F000000);
    OSScreenClearBufferEx(SCREEN_DRC, 0x7F000000);
    console_print_pos_va(25 - (strlen(message)>>1), 9, message, va);
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);
	va_end(va);
	sleep(2);	
}

void getAccountsWiiU() {
	/* get persistent ID - thanks to Maschell */
	nn::act::Initialize();
	int i = 0, accn = 0;
	wiiuaccn = nn::act::GetNumOfAccounts();
	wiiuacc = (Account*)malloc(wiiuaccn * sizeof(Account));
	uint16_t out[11];
	while ((accn < wiiuaccn) && (i <= 12)) {
		if (nn::act::IsSlotOccupied(i)) {
			unsigned int persistentID = nn::act::GetPersistentIdEx(i);
			wiiuacc[accn].pID = persistentID;
			sprintf(wiiuacc[accn].persistentID, "%08X", persistentID);
			nn::act::GetMiiNameEx((int16_t*)out, i);
			//convert_to_ascii(out, wiiuacc[accn].miiName);
			memset(wiiuacc[accn].miiName, 0, sizeof(wiiuacc[accn].miiName));
			for (int j = 0, k = 0; j < 10; j++) {
				if (out[j] < 0x80)
					wiiuacc[accn].miiName[k++] = (char)out[j];
				else if ((out[j] & 0xF000) > 0) {
					wiiuacc[accn].miiName[k++] = 0xE0 | ((out[j] & 0xF000) >> 12);
					wiiuacc[accn].miiName[k++] = 0x80 | ((out[j] & 0xFC0) >> 6);
					wiiuacc[accn].miiName[k++] = 0x80 | (out[j] & 0x3F);
				} else if (out[j] < 0x400) {
					wiiuacc[accn].miiName[k++] = 0xC0 | ((out[j] & 0x3C0) >> 6);
					wiiuacc[accn].miiName[k++] = 0x80 | (out[j] & 0x3F);
				} else {
					wiiuacc[accn].miiName[k++] = 0xD0 | ((out[j] & 0x3C0) >> 6);
					wiiuacc[accn].miiName[k++] = 0x80 | (out[j] & 0x3F);
				}
			}
			wiiuacc[accn].slot = i;
			accn++;
		}
		i++;
	}
	nn::act::Finalize();
}

void getAccountsSD(Title* title, u8 slot) {
	u32 highID = title->highID, lowID = title->lowID;
	int dirH;
	sdaccn = 0;
	if (sdacc) free(sdacc);

	char path[255];
	sprintf(path, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
	if (IOSUHAX_FSA_OpenDir(fsaFd, path, &dirH) >= 0) {
		while (1) {
			directoryEntry_s data;
			int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
			if (ret != 0) break;

			if (strncmp(data.name, "common", 6) == 0) continue;
			sdaccn++;
		}
		IOSUHAX_FSA_CloseDir(fsaFd, dirH);
	}

	sdacc = (Account*)malloc(sdaccn * sizeof(Account));
	if (IOSUHAX_FSA_OpenDir(fsaFd, path, &dirH) >= 0) {
		for(int i = 0; i < sdaccn; i++) {
			directoryEntry_s data;
			int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
			if (ret != 0) break;

			if (strncmp(data.name, "common", 6) == 0) continue;
			sprintf(sdacc[i].persistentID, "%s", data.name);
			sdacc[i].pID = strtoul(data.name, NULL, 16);
			sdacc[i].slot = i;
		}
		IOSUHAX_FSA_CloseDir(fsaFd, dirH);
	}
}

int DumpFile(char *pPath, const char * oPath)
{
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

	FILE* source = fopen(pPath, "rb");
    FILE* dest = fopen(oPath, "wb");
	if ((dest && source) == NULL) {
        return -1;
    }
	struct stat st;
	stat(pPath, &st);
	int sizef = st.st_size;
	int sizew = 0, size;
	u32 passedMs = 1;
	u64 startTime = OSGetTime();

	while ((size = fread(pBuffer, 1, buf_size, source)) > 0) {
		fwrite(pBuffer, 1, size, dest);
		passedMs = (uint32_t)OSTicksToMilliseconds(OSGetTime() - startTime);
        if(passedMs == 0)
            passedMs = 1; // avoid 0 div
		OSScreenClearBufferEx(SCREEN_TV, 0);
		OSScreenClearBufferEx(SCREEN_DRC, 0);
		sizew += size;
		show_file_operation("file", pPath, oPath);
		console_print_pos(-2, 15, "Bytes Copied: %d of %d (%i kB/s)", sizew, sizef,  (u32)(((u64)sizew * 1000) / ((u64)1024 * passedMs)));    
		flipBuffers();
	}

    fclose(source);
    fclose(dest);
	free(pBuffer);
	
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

		OSScreenClearBufferEx(SCREEN_TV, 0);
		OSScreenClearBufferEx(SCREEN_DRC, 0);

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
			show_file_operation(data.name, pPath, targetPath);

			if (DumpFile(pPath, targetPath) != 0) {
				IOSUHAX_FSA_CloseDir(fsaFd, dirH);
				return -3;
			}

			free(targetPath);
		}

		pPath[len] = 0;
	}

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

		OSScreenClearBufferEx(SCREEN_TV, 0);
		OSScreenClearBufferEx(SCREEN_DRC, 0);

		if (strcmp(data.name, "..") == 0 || strcmp(data.name, ".") == 0) continue;

		int len = strlen(pPath);
		snprintf(pPath + len, FS_MAX_FULLPATH_SIZE - len, "/%s", data.name);

		if (data.stat.flag & DIR_ENTRY_IS_DIRECTORY) {
			char origPath[PATH_SIZE];
			sprintf(origPath, "%s", pPath);
			DeleteDir(pPath);

			OSScreenClearBufferEx(SCREEN_TV, 0);
			OSScreenClearBufferEx(SCREEN_DRC, 0);

			console_print_pos(-2, 0, "Deleting folder %s", data.name);
			console_print_pos_multiline(-2, 2, '/', "From: \n%s", origPath);
			if (IOSUHAX_FSA_Remove(fsaFd, origPath) != 0) promptError("Failed to delete folder.");
		} else {
			console_print_pos(-2, 0, "Deleting file %s", data.name);
			console_print_pos_multiline(-2, 2, '/', "From: \n%s", pPath);
			if (IOSUHAX_FSA_Remove(fsaFd, pPath) != 0) promptError("Failed to delete file.");
		}

		flipBuffers();
		pPath[len] = 0;
	}

	IOSUHAX_FSA_CloseDir(fsaFd, dirH);
	return 0;
}

void getUserID(char* out) { // Source: loadiine_gx2
	/* get persistent ID - thanks to Maschell */
	nn::act::Initialize();

	unsigned char slotno = nn::act::GetSlotNo();
	unsigned int persistentID = nn::act::GetPersistentIdEx(slotno);
	nn::act::Finalize();

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
		promptError("Failed to open Loadiine game save directory.");
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
	if (checkEntry(out) <= 0) return -1;
	IOSUHAX_FSA_CloseDir(fsaFd, dirH);
	return 0;
}

u64 getSlotDate(u32 highID, u32 lowID, u8 slot) {
	char path[PATH_SIZE];
	if (((highID & 0xFFFFFFF0) == 0x00010000) && (slot == 255)) {
		sprintf(path, "/vol/storage_sdcard/savegames/%08x%08x", highID, lowID);
	} else {
		sprintf(path, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
	}
	int ret = checkEntry(path);
	if (ret <= 0) return 0;
	else {
		fileStat_s fStat;
		ret = FSAR(IOSUHAX_FSA_GetStat(fsaFd, path, &fStat));
		return fStat.ctime;
	}
}

bool isSlotEmpty(u32 highID, u32 lowID, u8 slot) {
	char path[PATH_SIZE];
	if (((highID & 0xFFFFFFF0) == 0x00010000) && (slot == 255)) {
		sprintf(path, "/vol/storage_sdcard/savegames/%08x%08x", highID, lowID);
	} else {
		sprintf(path, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
	}
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

bool hasAccountSave(Title* title, bool inSD, bool iine, u32 user, u8 slot, int version) {
	u32 highID = title->highID, lowID = title->lowID;
	bool isUSB = title->isTitleOnUSB, isWii = ((highID & 0xFFFFFFF0) == 0x00010000);
	if (highID == 0 || lowID == 0) return false;

	char srcPath[PATH_SIZE];
	if (!isWii) {
		if (!inSD) {
			const char* path = (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save");
			if (user == 0)
				sprintf(srcPath, "%s/%08x/%08x/%s/common", path, highID, lowID, "user");
			else if (user == 0xFFFFFFFF)
				sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, "user");
			else
				sprintf(srcPath, "%s/%08x/%08x/%s/%08X", path, highID, lowID, "user", user);
		} else {
			if (!iine)
				sprintf(srcPath, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u/%08X", highID, lowID, slot, user);
			else {
				if (getLoadiineGameSaveDir(srcPath, title->productCode) != 0) return false;
				if (version) sprintf(srcPath + strlen(srcPath), "/v%u", version);
				if (user == 0) {
					u32 srcOffset = strlen(srcPath);
					strcpy(srcPath + srcOffset, "/c\0");
				} else {
					char usrPath[16];
					sprintf(usrPath, "%08X", user);
					getLoadiineUserDir(srcPath, srcPath, usrPath);
				}

			}
		}
	} else {
		if (!inSD) {
			sprintf(srcPath, "/vol/storage_slccmpt01/title/%08x/%08x/data", highID, lowID);
		} else {
			sprintf(srcPath, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
		}
	}
	if (checkEntry(srcPath) == 2)
		if (folderEmpty(srcPath) == 0)
			return true;
	return false;
}

bool hasCommonSave(Title* title, bool inSD, bool iine, u8 slot, int version) {
	u32 highID = title->highID, lowID = title->lowID;
	bool isUSB = title->isTitleOnUSB, isWii = ((highID & 0xFFFFFFF0) == 0x00010000);
	if (isWii) return false;

	char srcPath[PATH_SIZE];
	if (!inSD) {
		const char* path = (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save");
		sprintf(srcPath, "%s/%08x/%08x/%s/common", path, highID, lowID, "user");
	} else {
		if (!iine)
			sprintf(srcPath, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u/common", highID, lowID, slot);
		else {
			if (getLoadiineGameSaveDir(srcPath, title->productCode) != 0) return false;
			if (version) sprintf(srcPath + strlen(srcPath), "/v%u", version);
			u32 srcOffset = strlen(srcPath);
			strcpy(srcPath + srcOffset, "/c\0");
		}
	}
	if (checkEntry(srcPath) == 2)
		if (folderEmpty(srcPath) == 0) return true;
	return false;
}

void copySavedata(Title* title, Title* titleb, s8 allusers, s8 allusers_d, bool common) {

	u32 highID = title->highID, lowID = title->lowID;
	bool isUSB = title->isTitleOnUSB;
	u32 highIDb = titleb->highID, lowIDb = titleb->lowID;
	bool isUSBb = titleb->isTitleOnUSB;

	if (!promptConfirm(ST_WARNING, "Are you sure?")) return;
	int slotb = getEmptySlot(titleb->highID, titleb->lowID);
	if ((slotb >= 0) && promptConfirm(ST_YES_NO, "Backup current savedata first to next empty slot?")) {
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

	if (allusers > -1) {
		u32 srcOffset = strlen(srcPath);
		u32 dstOffset = strlen(dstPath);
		if (common) {
			strcpy(srcPath + srcOffset, "/common");
			strcpy(dstPath + dstOffset, "/common");
			if (DumpDir(srcPath, dstPath) != 0) promptError("Common save not found.");
		}
		sprintf(srcPath + srcOffset, "/%s", wiiuacc[allusers].persistentID);
		sprintf(dstPath + dstOffset, "/%s", wiiuacc[allusers_d].persistentID);
	}

	if (DumpDir(srcPath, dstPath) != 0) promptError("Copy failed.");

	if (strncmp(strchr(dstPath, '_'), "_usb", 4) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_usb01");
	} else if (strncmp(strchr(dstPath, '_'), "_mlc", 4) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_mlc01");
	} else if (strncmp(strchr(dstPath, '_'), "_slccmpt", 8) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_slccmpt01");
	} else if (strncmp(strchr(dstPath, '_'), "_sdcard", 7) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_sdcard");
	}
}

void backupAllSave(Title* titles, int count, OSCalendarTime* date) {
	OSCalendarTime dateTime;
	if (date) {
		if (date->tm_year == 0) {
			OSTicksToCalendarTime(OSGetTime(), date);
			date->tm_mon++;
		} dateTime = (*date);
	} else {
		OSTicksToCalendarTime(OSGetTime(), &dateTime);
		dateTime.tm_mon++;
	}

	char datetime[24];
	sprintf(datetime, "%04d-%02d-%02dT%02d%02d%02d", dateTime.tm_year, dateTime.tm_mon, dateTime.tm_mday, dateTime.tm_hour, dateTime.tm_min, dateTime.tm_sec);
	for (int i = 0; i < count; i++) {
		if (titles[i].highID == 0 || titles[i].lowID == 0 || !titles[i].saveInit) continue;

		u32 highID = titles[i].highID, lowID = titles[i].lowID;
		bool isUSB = titles[i].isTitleOnUSB, isWii = ((highID & 0xFFFFFFF0) == 0x00010000);
		char srcPath[PATH_SIZE];
		char dstPath[PATH_SIZE];
		const char* path = (isWii ? "/vol/storage_slccmpt01/title" : (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save"));
		sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, isWii ? "data" : "user");
		sprintf(dstPath, "/vol/storage_sdcard/wiiu/backups/batch/%s/%08x%08x", datetime, highID, lowID);

		createFolder(dstPath);
		if (DumpDir(srcPath, dstPath) != 0) promptError("Backup failed.");
	}
}

void backupSavedata(Title* title, u8 slot, s8 allusers, bool common) {

	if (!isSlotEmpty(title->highID, title->lowID, slot) && !promptConfirm(ST_WARNING, "Backup found on this slot. Overwrite it?")) return;
	u32 highID = title->highID, lowID = title->lowID;
	bool isUSB = title->isTitleOnUSB, isWii = ((highID & 0xFFFFFFF0) == 0x00010000);
	char srcPath[PATH_SIZE];
	char dstPath[PATH_SIZE];
	const char* path = (isWii ? "/vol/storage_slccmpt01/title" : (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save"));
	sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, isWii ? "data" : "user");
	if (isWii && (slot == 255)) {
		sprintf(dstPath, "/vol/storage_sdcard/savegames/%08x%08x", highID, lowID);
	} else {
		sprintf(dstPath, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
	}
	createFolder(dstPath);

	if ((allusers > -1) && !isWii) {
		u32 srcOffset = strlen(srcPath);
		u32 dstOffset = strlen(dstPath);
		if (common) {
			strcpy(srcPath + srcOffset, "/common");
			strcpy(dstPath + dstOffset, "/common");
			if (DumpDir(srcPath, dstPath) != 0) promptError("Common save not found.");
		}
		sprintf(srcPath + srcOffset, "/%s", wiiuacc[allusers].persistentID);
		sprintf(dstPath + dstOffset, "/%s", wiiuacc[allusers].persistentID);
		if (checkEntry(srcPath) == 0) {
			promptError("No save found for this user.");
			return;
		}
	}

	if (DumpDir(srcPath, dstPath) != 0) promptError("Backup failed. DO NOT restore from this slot.");

	if (strncmp(strchr(dstPath, '_'), "_usb", 4) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_usb01");
	} else if (strncmp(strchr(dstPath, '_'), "_mlc", 4) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_mlc01");
	} else if (strncmp(strchr(dstPath, '_'), "_slccmpt", 8) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_slccmpt01");
	} else if (strncmp(strchr(dstPath, '_'), "_sdcard", 7) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_sdcard");
	}
}

void restoreSavedata(Title* title, u8 slot, s8 sdusers, s8 allusers, bool common) {

	if (isSlotEmpty(title->highID, title->lowID, slot)) {
		promptError("No backup found on selected slot.");
		return;
	}
	if (!promptConfirm(ST_WARNING, "Are you sure?")) return;
	int slotb = getEmptySlot(title->highID, title->lowID);
	if ((slotb >= 0) && promptConfirm(ST_YES_NO, "Backup current savedata first to next empty slot?")) backupSavedata(title, slotb, allusers, common);
	u32 highID = title->highID, lowID = title->lowID;
	bool isUSB = title->isTitleOnUSB, isWii = ((highID & 0xFFFFFFF0) == 0x00010000);
	char srcPath[PATH_SIZE];
	char dstPath[PATH_SIZE];
	const char* path = (isWii ? "/vol/storage_slccmpt01/title" : (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save"));
	if (isWii && (slot == 255)) {
		sprintf(srcPath, "/vol/storage_sdcard/savegames/%08x%08x", highID, lowID);
	} else {
		sprintf(srcPath, "/vol/storage_sdcard/wiiu/backups/%08x%08x/%u", highID, lowID, slot);
	}
	sprintf(dstPath, "%s/%08x/%08x/%s", path, highID, lowID, isWii ? "data" : "user");
	createFolder(dstPath);

	if ((sdusers > -1) && !isWii) {
		u32 srcOffset = strlen(srcPath);
		u32 dstOffset = strlen(dstPath);
		if (common) {
			strcpy(srcPath + srcOffset, "/common");
			strcpy(dstPath + dstOffset, "/common");
			if (DumpDir(srcPath, dstPath) != 0) promptError("Common save not found.");
		}
		sprintf(srcPath + srcOffset, "/%s", sdacc[sdusers].persistentID);
		sprintf(dstPath + dstOffset, "/%s", wiiuacc[allusers].persistentID);
	}

	if (DumpDir(srcPath, dstPath) != 0) promptError("Restore failed.");

	if (strncmp(strchr(dstPath, '_'), "_usb", 4) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_usb01");
	} else if (strncmp(strchr(dstPath, '_'), "_mlc", 4) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_mlc01");
	} else if (strncmp(strchr(dstPath, '_'), "_slccmpt", 8) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_slccmpt01");
	} else if (strncmp(strchr(dstPath, '_'), "_sdcard", 7) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_sdcard");
	}
}

void wipeSavedata(Title* title, s8 allusers, bool common) {

	if (!promptConfirm(ST_WARNING, "Are you sure?") || !promptConfirm(ST_WARNING, "Hm, are you REALLY sure?")) return;
	int slotb = getEmptySlot(title->highID, title->lowID);
	if ((slotb >= 0) && promptConfirm(ST_YES_NO, "Backup current savedata first?")) backupSavedata(title, slotb, allusers, common);
	u32 highID = title->highID, lowID = title->lowID;
	bool isUSB = title->isTitleOnUSB, isWii = ((highID & 0xFFFFFFF0) == 0x00010000);
	char srcPath[PATH_SIZE];
	char origPath[PATH_SIZE];
	const char* path = (isWii ? "/vol/storage_slccmpt01/title" : (isUSB ? "/vol/storage_usb01/usr/save" : "/vol/storage_mlc01/usr/save"));
	sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, isWii ? "data" : "user");
	if ((allusers > -1) && !isWii) {
		u32 offset = strlen(srcPath);
		if (common) {
			strcpy(srcPath + offset, "/common");
			sprintf(origPath, "%s", srcPath);
			if (DeleteDir(srcPath) != 0) promptError("Common save not found.");
			if (IOSUHAX_FSA_Remove(fsaFd, origPath) != 0) promptError("Failed to delete common folder.");
		}
		sprintf(srcPath + offset, "/%s", wiiuacc[allusers].persistentID);
		sprintf(origPath, "%s", srcPath);
	}

	if (DeleteDir(srcPath)!=0) promptError("Failed to delete savefile.");
	if ((allusers > -1) && !isWii) {
		if (IOSUHAX_FSA_Remove(fsaFd, origPath) != 0) promptError("Failed to delete user folder.");
	}

	if (strncmp(strchr(srcPath, '_'), "_usb", 4) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_usb01");
	} else if (strncmp(strchr(srcPath, '_'), "_mlc", 4) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_mlc01");
	} else if (strncmp(strchr(srcPath, '_'), "_slccmpt", 8) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_slccmpt01");
	} else if (strncmp(strchr(srcPath, '_'), "_sdcard", 7) == 0) {
		IOSUHAX_FSA_FlushVolume(fsaFd, "/vol/storage_sdcard");
	}
}

void importFromLoadiine(Title* title, bool common, int version) {

	if (!promptConfirm(ST_WARNING, "Are you sure?")) return;
	int slotb = getEmptySlot(title->highID, title->lowID);
	if (slotb>=0 && promptConfirm(ST_YES_NO, "Backup current savedata first?")) backupSavedata(title, slotb, 0, common);
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
	promptError(srcPath);
	promptError(dstPath);
	if (DumpDir(srcPath, dstPath) != 0) promptError("Failed to import savedata from loadiine.");
	if (common) {
		strcpy(srcPath + srcOffset, "/c\0");
		strcpy(dstPath + dstOffset, "/common\0");
		promptError(srcPath);
		promptError(dstPath);
		if (DumpDir(srcPath, dstPath) != 0) promptError("Common save not found.");
	}
}

void exportToLoadiine(Title* title, bool common, int version) {

	if (!promptConfirm(ST_WARNING, "Are you sure?")) return;
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
	createFolder(dstPath);
	promptError(srcPath);
	promptError(dstPath);
	if (DumpDir(srcPath, dstPath) != 0) promptError("Failed to export savedata to loadiine.");
	if (common) {
		strcpy(dstPath + dstOffset, "/c\0");
		strcpy(srcPath + srcOffset, "/common\0");
		promptError(srcPath);
		promptError(dstPath);
		if (DumpDir(srcPath, dstPath) != 0) promptError("Common save not found.");
	}
}
