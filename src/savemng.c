#include <sys/dirent.h>

#include "savemng.h"

#define BUFFER_SIZE 0x80000

void console_print_pos(int x, int y, const char *format, ...) {

	char * tmp = NULL;

	va_list va;
	va_start(va, format);
	if ((vasprintf(&tmp, format, va) >= 0) && tmp) {

        if(strlen(tmp) > 79) tmp[79] = 0;

        OSScreenPutFontEx(0, x, y, tmp);
        OSScreenPutFontEx(1, x, y, tmp);

	}

	va_end(va);
	if(tmp) free(tmp);

}

int DumpFile(char *pPath, const char * output_path) {

    char *pFilename = strrchr(pPath, '/');
    if(!pFilename) pFilename = pPath;
    else pFilename++;

    unsigned char* dataBuf = (unsigned char*)memalign(0x40, BUFFER_SIZE);
    if(!dataBuf) {
        promptError("Out of memory.");
        return -1;
    }

    FILE *pReadFile = fopen(pPath, "rb");
    if(!pReadFile) {
        promptError("Failed to open file.");
        return -2;
    }

    FILE *pWriteFile = fopen(output_path, "wb");
    if(!pWriteFile) {
        promptError("Failed to create file.");
        fclose(pReadFile);
        return -3;
    }

    unsigned int size = 0;
    unsigned int ret;

    // Copy rpl in memory
    while ((ret = fread(dataBuf, 0x1, BUFFER_SIZE, pReadFile)) > 0) {
        fwrite(dataBuf, 0x01, ret, pWriteFile);
        size += ret;
    }

    fclose(pWriteFile);
    fclose(pReadFile);
    free(dataBuf);

    return 0;

}

int DumpDir(char *pPath, const char * target_path) {

    struct dirent *dirent = NULL;
    DIR *dir = NULL;

    dir = opendir(pPath);
    if (dir == NULL) {
        promptError("Failed to open directory.");
        return -1;
    }

    CreateSubfolder(target_path);

    while ((dirent = readdir(dir)) != 0) {

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);

        if(strcmp(dirent->d_name, "..") == 0 || strcmp(dirent->d_name, ".") == 0) continue;

        int len = strlen(pPath);
        snprintf(pPath + len, FS_MAX_FULLPATH_SIZE - len, "/%s", dirent->d_name);

        if(dirent->d_type & DT_DIR) {

            char *targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
            snprintf(targetPath, FS_MAX_FULLPATH_SIZE, "%s/%s", target_path, dirent->d_name);

            CreateSubfolder(targetPath);
            DumpDir(pPath, targetPath);
            free(targetPath);

        } else {

            char *targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
            snprintf(targetPath, FS_MAX_FULLPATH_SIZE, "%s/%s", target_path, dirent->d_name);

            console_print_pos(0, 0, "Copying file %s", dirent->d_name);
            console_print_pos(0, 1, "From: %s", pPath);
            console_print_pos(0, 2, "To: %s", targetPath);

            DumpFile(pPath, targetPath);
            free(targetPath);

        }

        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);

        pPath[len] = 0;

    }

    closedir(dir);

    return 0;

}

void backupSavedata(u32 highID, u32 lowID, bool isUSB, int slot) {

    char srcPath[256];
    char dstPath[256];
    sprintf(srcPath, "storage_%s:/usr/save/%08x/%08x/user", isUSB ? "usb" : "mlc", highID, lowID);
    sprintf(dstPath, "sd:/wiiu/apps/savemii/backups/%08x%08x/%i", highID, lowID, slot);
    DumpDir(srcPath, dstPath);

}

void restoreSavedata(u32 highID, u32 lowID, bool isUSB, int slot) {

    char srcPath[256];
    char dstPath[256];
    sprintf(srcPath, "sd:/wiiu/apps/savemii/backups/%08x%08x/%i", highID, lowID, slot);
    sprintf(dstPath, "storage_%s:/usr/save/%08x/%08x/user", isUSB ? "usb" : "mlc", highID, lowID);
    DumpDir(srcPath, dstPath);

}

void wipeSavedata(u32 highID, u32 lowID, bool isUSB) {
    
}