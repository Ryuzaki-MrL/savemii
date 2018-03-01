#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>

#include "main.h"
#include "savemng.h"
#include "icon.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 2
#define VERSION_MICRO 0
#define VERSION_MOD "mod5"
#define M_OFF 3

u8 slot = 0;
s8 allusers = -1, allusers_d = -1, sdusers = -1;
bool common = 1;
int menu = 0, mode = 0, task = 0, targ = 0, tsort = 1, sorta = 1;
int cursor = 0, scroll = 0;
int cursorb = 0, cursort = 0, scrollb = 0;
int titleswiiu = 0, titlesvwii = 0;
const char *sortn[4] = {"None", "Name", "Storage", "Storage+Name"};

//just to be able to call async
void someFunc(void *arg) {
    (void)arg;
}

static int mcp_hook_fd = -1;
int MCPHookOpen() {
    //take over mcp thread
    mcp_hook_fd = MCP_Open();
    if (mcp_hook_fd < 0) return -1;
    IOS_IoctlAsync(mcp_hook_fd, 0x62, (void*)0, 0, (void*)0, 0, someFunc, (void*)0);
    //let wupserver start up
    os_sleep(1);
    if (IOSUHAX_Open("/dev/mcp") < 0) {
        MCP_Close(mcp_hook_fd);
        mcp_hook_fd = -1;
        return -1;
    }
    return 0;
}

void MCPHookClose() {
    if (mcp_hook_fd < 0) return;
    //close down wupserver, return control to mcp
    IOSUHAX_Close();
    //wait for mcp to return
    os_sleep(1);
    MCP_Close(mcp_hook_fd);
    mcp_hook_fd = -1;
}

int titleSort(const void *c1, const void *c2)
{
    if (tsort == 0) {
        return ((Title*)c1)->listID - ((Title*)c2)->listID;
    } else if (tsort == 1) {
        return strcmp(((Title*)c1)->shortName,((Title*)c2)->shortName) * sorta;
    } else if (tsort == 2) {
        if (((Title*)c1)->isTitleOnUSB == ((Title*)c2)->isTitleOnUSB)
            return 0;
        if (((Title*)c1)->isTitleOnUSB)
            return -1 * sorta;
        if (((Title*)c2)->isTitleOnUSB)
            return 1 * sorta;
        return 0;
    } else if (tsort == 3) {
        if (((Title*)c1)->isTitleOnUSB && !((Title*)c2)->isTitleOnUSB)
            return -1 * sorta;
        if (!((Title*)c1)->isTitleOnUSB && ((Title*)c2)->isTitleOnUSB)
            return 1 * sorta;

        return strcmp(((Title*)c1)->shortName,((Title*)c2)->shortName) * sorta;
    }
}

Title* loadWiiUTitles(int run, int fsaFd) {
    static char *tList;
    static int receivedCount;
    // Source: haxchi installer
    if (run == 0) {
        int mcp_handle = MCP_Open();
        int count = MCP_TitleCount(mcp_handle);
        int listSize = count*0x61;
        tList = memalign(32, listSize);
        memset(tList, 0, listSize);
        receivedCount = count;
        MCP_TitleList(mcp_handle, &receivedCount, tList, listSize);
        MCP_Close(mcp_handle);
        return NULL;
    }

    int usable = receivedCount, j = 0;
    Saves* savesl = malloc(receivedCount*sizeof(Saves));
    if (!savesl) {
        promptError("Out of memory.");
        return NULL;
    }
    for (int i = 0; i < receivedCount; i++) {
        char* element = tList+(i*0x61);
        savesl[j].highID = *(u32*)(element);
        if (savesl[j].highID != 0x00050000) {
            usable--;
            continue;
        }
        savesl[j].lowID = *(u32*)(element+4);
        savesl[j].dev = !(memcmp(element+0x56, "usb", 4) == 0);
        savesl[j].found = false;
        j++;
    }
    savesl = realloc(savesl, usable * sizeof(Saves));

    int dirUH, dirNH, foundCount = 0, pos = 0, tNoSave = usable;
    for (int i = 0; i <= 1; i++) {
        char path[255];
        sprintf(path, "/vol/storage_%s01/usr/save/00050000", (i == 0) ? "usb" : "mlc");
        if (IOSUHAX_FSA_OpenDir(fsaFd, path, &dirUH) >= 0) {
            while (1) {
                directoryEntry_s data;
        		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirUH, &data);
        		if (ret != 0)
        			break;

                sprintf(path, "/vol/storage_%s01/usr/save/00050000/%s/user", (i == 0) ? "usb" : "mlc", data.name);
                if (checkEntry(path) == 2) {
                    sprintf(path, "/vol/storage_%s01/usr/save/00050000/%s/meta/meta.xml", (i == 0) ? "usb" : "mlc", data.name);
                    if (checkEntry(path) == 1) {
                        for (int i = 0; i < usable; i++) {
                            if ((savesl[i].highID == 0x00050000) && (strtoul(data.name, NULL, 16) == savesl[i].lowID)) {
                                savesl[i].found = true;
                                tNoSave--;
                                break;
                            }
                        }
                        foundCount++;
                    }
                }
            }
            IOSUHAX_FSA_CloseDir(fsaFd, dirUH);
        }
    }

    foundCount += tNoSave;
    Saves* saves = malloc((foundCount + tNoSave)*sizeof(Saves));
    if (!saves) {
        promptError("Out of memory.");
        return NULL;
    }

    for (int i = 0; i <= 1; i++) {
        char path[255];
        sprintf(path, "/vol/storage_%s01/usr/save/00050000", (i == 0) ? "usb" : "mlc");
        if (IOSUHAX_FSA_OpenDir(fsaFd, path, &dirUH) >= 0) {
            while (1) {
                directoryEntry_s data;
        		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirUH, &data);
        		if (ret != 0)
        			break;

                sprintf(path, "/vol/storage_%s01/usr/save/00050000/%s/meta/meta.xml", (i == 0) ? "usb" : "mlc", data.name);
                if (checkEntry(path) == 1) {
                    saves[pos].highID = 0x00050000;
                    saves[pos].lowID = strtoul(data.name, NULL, 16);
                    saves[pos].dev = i;
                    saves[pos].found = false;
                    pos++;
                }
            }
            IOSUHAX_FSA_CloseDir(fsaFd, dirUH);
        }
    }

    for (int i = 0; i < usable; i++) {
        if (!savesl[i].found) {
            saves[pos].highID = savesl[i].highID;
            saves[pos].lowID = savesl[i].lowID;
            saves[pos].dev = savesl[i].dev;
            saves[pos].found = true;
            pos++;
        }
    }

    Title* titles = malloc(foundCount*sizeof(Title));
    if (!titles) {
        promptError("Out of memory.");
        return NULL;
    }

    for (int i = 0; i < foundCount; i++) {
        int srcFd = -1;
        u32 highID = saves[i].highID, lowID = saves[i].lowID;
        bool isTitleOnUSB = !saves[i].dev;

        char path[255];
        memset(path, 0, 255);
        if (saves[i].found)
            sprintf(path, "/vol/storage_%s01/usr/title/%08x/%08x/meta/meta.xml", isTitleOnUSB ? "usb" : "mlc", highID, lowID);
        else
            sprintf(path, "/vol/storage_%s01/usr/save/%08x/%08x/meta/meta.xml", isTitleOnUSB ? "usb" : "mlc", highID, lowID);
        titles[titleswiiu].saveInit = !saves[i].found;

        char* xmlBuf = NULL;
        if (loadFile(path, &xmlBuf) > 0) {
            char *cptr = strchr(strstr(xmlBuf, "product_code"), '>') + 7;
            memset(titles[titleswiiu].productCode, 0, sizeof(titles[titleswiiu].productCode));
            strncpy(titles[titleswiiu].productCode, cptr, strcspn(cptr, "<"));

            cptr = strchr(strstr(xmlBuf, "shortname_en"), '>') + 1;
            memset(titles[titleswiiu].shortName, 0, sizeof(titles[titleswiiu].shortName));
            if (strcspn(cptr, "<") == 0)
                cptr = strchr(strstr(xmlBuf, "shortname_ja"), '>') + 1;
            strncpy(titles[titleswiiu].shortName, cptr, strcspn(cptr, "<"));

            cptr = strchr(strstr(xmlBuf, "longname_en"), '>') + 1;
            memset(titles[i].longName, 0, sizeof(titles[i].longName));
            if (strcspn(cptr, "<") == 0)
                cptr = strchr(strstr(xmlBuf, "longname_ja"), '>') + 1;
            strncpy(titles[titleswiiu].longName, cptr, strcspn(cptr, "<"));

            free(xmlBuf);
        }

	    titles[titleswiiu].isTitleDupe = false;
	    for (int i = 0; i < titleswiiu; i++) {
		    if ((titles[i].highID == highID) && (titles[i].lowID == lowID)) {
			    titles[titleswiiu].isTitleDupe = true;
			    titles[titleswiiu].dupeID = i;
			    titles[i].isTitleDupe = true;
			    titles[i].dupeID = titleswiiu;
		    }
	    }

        titles[titleswiiu].highID = highID;
        titles[titleswiiu].lowID = lowID;
        titles[titleswiiu].isTitleOnUSB = isTitleOnUSB;
        titles[titleswiiu].listID = titleswiiu;
        if (loadTitleIcon(&titles[titleswiiu]) < 0) titles[titleswiiu].iconBuf = NULL;
        titleswiiu++;

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);
        drawTGA(298, 144, 1, icon_tga);
        console_print_pos_aligned(10, 0, 1, "Loaded %i Wii U titles.", titleswiiu);
        flipBuffers();

    }

    free(savesl);
    free(saves);
    free(tList);
    return titles;

}

Title* loadWiiTitles(int fsaFd) {
    int dirH;
    char* highIDs[3] = {"00010000", "00010001", "00010004"};
    bool found = false;
    u32* blacklist[7][2] = {{0x00010000, 0x00555044}, {0x00010000, 0x00555045}, \
                            {0x00010000, 0x0055504A}, {0x00010000, 0x524F4E45}, \
                            {0x00010000, 0x52543445}, {0x00010001, 0x48424344},
                            {0x00010001, 0x554E454F}};

    char pathW[256];
    for (int k = 0; k < 3; k++) {
        sprintf(pathW, "/vol/storage_slccmpt01/title/%s", highIDs[k]);
        if (IOSUHAX_FSA_OpenDir(fsaFd, pathW, &dirH) >= 0) {
            while (1) {
                directoryEntry_s data;
        		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
        		if (ret != 0) break;
                for (int ii = 0; ii < 7; ii++) {
                    if (blacklist[ii][0] == strtoul(highIDs[k], NULL, 16)) {
                        if (blacklist[ii][1] == strtoul(data.name, NULL, 16)) {found = true; break;}
                    }
                } if (found) {found = false; continue;}

                titlesvwii++;
            } IOSUHAX_FSA_CloseDir(fsaFd, dirH);
        }
    }
    if (titlesvwii == 0) return NULL;

    Title* titles = malloc(titlesvwii * sizeof(Title));
    if (!titles) {
        promptError("Out of memory.");
        return NULL;
    }

    int i = 0;
    for (int k = 0; k < 3; k++) {
        sprintf(pathW, "/vol/storage_slccmpt01/title/%s", highIDs[k]);
        if (IOSUHAX_FSA_OpenDir(fsaFd, pathW, &dirH) >= 0) {
            while (1) {
                directoryEntry_s data;
        		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
        		if (ret != 0) break;
                for (int ii = 0; ii < 7; ii++) {
                    if (blacklist[ii][0] == strtoul(highIDs[k], NULL, 16)) {
                        if (blacklist[ii][1] == strtoul(data.name, NULL, 16)) {found = true; break;}
                    }
                } if (found) {found = false; continue;}

                int srcFd = -1;
                char path[256];
                sprintf(path, "/vol/storage_slccmpt01/title/%s/%s/data/banner.bin", highIDs[k], data.name);
                ret = IOSUHAX_FSA_OpenFile(fsaFd, path, "rb", &srcFd);
                if (ret >= 0) {
                    IOSUHAX_FSA_SetFilePos(fsaFd, srcFd, 0x20);
                    u16* bnrBuf = (u16*)malloc(0x80);
                    if (bnrBuf) {
                        IOSUHAX_FSA_ReadFile(fsaFd, bnrBuf, 0x02, 0x40, srcFd, 0);
                        IOSUHAX_FSA_CloseFile(fsaFd, srcFd);

                        memset(titles[i].shortName, 0, sizeof(titles[i].shortName));
                        for (int j = 0, k = 0; j < 0x20; j++) {
                            if (bnrBuf[j] < 0x80)
                                titles[i].shortName[k++] = (char)bnrBuf[j];
                            else if ((bnrBuf[j] & 0xF000) > 0) {
                                titles[i].shortName[k++] = 0xE0 | ((bnrBuf[j] & 0xF000) >> 12);
                                titles[i].shortName[k++] = 0x80 | ((bnrBuf[j] & 0xFC0) >> 6);
                                titles[i].shortName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            } else if (bnrBuf[j] < 0x400) {
                                titles[i].shortName[k++] = 0xC0 | ((bnrBuf[j] & 0x3C0) >> 6);
                                titles[i].shortName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            } else {
                                titles[i].shortName[k++] = 0xD0 | ((bnrBuf[j] & 0x3C0) >> 6);
                                titles[i].shortName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            }
                        }

                        memset(titles[i].longName, 0, sizeof(titles[i].longName));
                        for (int j = 0x20, k = 0; j < 0x40; j++) {
                            if (bnrBuf[j] < 0x80)
                                titles[i].longName[k++] = (char)bnrBuf[j];
                            else if ((bnrBuf[j] & 0xF000) > 0) {
                                titles[i].longName[k++] = 0xE0 | ((bnrBuf[j] & 0xF000) >> 12);
                                titles[i].longName[k++] = 0x80 | ((bnrBuf[j] & 0xFC0) >> 6);
                                titles[i].longName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            } else if (bnrBuf[j] < 0x400) {
                                titles[i].longName[k++] = 0xC0 | ((bnrBuf[j] & 0x3C0) >> 6);
                                titles[i].longName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            } else {
                                titles[i].longName[k++] = 0xD0 | ((bnrBuf[j] & 0x3C0) >> 6);
                                titles[i].longName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            }
                        }

                        free(bnrBuf);
                        titles[i].saveInit = true;
                    }
                } else {
                    sprintf(titles[i].shortName, "%s%s (No banner.bin)", highIDs[k], data.name);
                    memset(titles[i].longName, 0, sizeof(titles[i].longName));
                    titles[i].saveInit = false;
                }

                titles[i].highID = strtoul(highIDs[k], NULL, 16);
                titles[i].lowID = strtoul(data.name, NULL, 16);

                titles[i].listID = i;
                memcpy(titles[i].productCode, &titles[i].lowID, 4);
                for (int ii = 0; ii < 4; ii++) {
                    if (titles[i].productCode[ii] == 0) titles[i].productCode[ii] = '.';
                }
                titles[i].productCode[4] = 0;
                titles[i].isTitleOnUSB = false;
                titles[i].isTitleDupe = false;
                titles[i].dupeID = 0;
                if (!titles[i].saveInit || (loadTitleIcon(&titles[i]) < 0)) titles[i].iconBuf = NULL;
                i++;

                OSScreenClearBufferEx(0, 0);
                OSScreenClearBufferEx(1, 0);
                drawTGA(298, 144, 1, icon_tga);
                console_print_pos_aligned(10, 0, 1, "Loaded %i Wii U titles.", titleswiiu);
                console_print_pos_aligned(11, 0, 1, "Loaded %i Wii titles.", i);
                flipBuffers();
            }
            IOSUHAX_FSA_CloseDir(fsaFd, dirH);
        }
    }

    return titles;
}

void unloadTitles(Title* titles, int count) {
    for (int i = 0; i < count; i++) {
        if (titles[i].iconBuf) free(titles[i].iconBuf);
    }
    if (titles) free(titles);
}

void disclaimer() {
    fillScreen(0, 0, 0, 0);
    console_print_pos_aligned(3, 0, 1, "Savemii Mod");
    //drawTGA(298, 36, 1, icon_tga);
    console_print_pos_aligned(6, 0, 1, "Created by: Ryuzaki-MrL");
    console_print_pos_aligned(7, 0, 1, "Modded by: GabyPCgeeK");
    console_print_pos_aligned(11, 0, 1, "Disclaimer:");
    console_print_pos_aligned(12, 0, 1, "There is always the potential for a brick.");
    console_print_pos_aligned(13, 0, 1, "Everything you do with this software is your own responsibility");
    flipBuffers();
    os_sleep(4);
}

/* Entry point */
int Menu_Main(void) {

    //mount_sd_fat("sd");
    loadWiiUTitles(0, -1);

    int res = IOSUHAX_Open(NULL);
    if (res < 0) {
        res = MCPHookOpen();
    }
    if (res < 0) {
        promptError("IOSUHAX_Open failed.");
        //unmount_sd_fat("sd");
        return EXIT_SUCCESS;
    }

    fatInitDefault();

    int fsaFd = IOSUHAX_FSA_Open();
    if (fsaFd < 0) {
        promptError("IOSUHAX_FSA_Open failed.");
        //unmount_sd_fat("sd");
        if (mcp_hook_fd >= 0) MCPHookClose();
        else IOSUHAX_Close();
        return EXIT_SUCCESS;
    }
    setFSAFD(fsaFd);

    IOSUHAX_FSA_Mount(fsaFd, "/dev/sdcard01", "/vol/storage_sdcard", 2, (void*)0, 0);
    mount_fs("sd", fsaFd, NULL, "/vol/storage_sdcard");
    mount_fs("slccmpt01", fsaFd, "/dev/slccmpt01", "/vol/storage_slccmpt01");
    mount_fs("storage_mlc", fsaFd, NULL, "/vol/storage_mlc01");
    mount_fs("storage_usb", fsaFd, NULL, "/vol/storage_usb01");
    mount_fs("storage_odd", fsaFd, "/dev/odd03", "/vol/storage_odd_content");

    u8* fontBuf = NULL;
    s32 fsize = loadFile("/vol/storage_sdcard/wiiu/apps/savemii_mod/font.ttf", &fontBuf);
    if (fsize > 0) {
        initFont(fontBuf, fsize);
    } else initFont(NULL, 0);

    disclaimer();
    clearBuffers();
    Title* wiiutitles = loadWiiUTitles(1, fsaFd);
    Title* wiititles = loadWiiTitles(fsaFd);
    int* versionList = (int*)malloc(0x100 * sizeof(int));
    os_sleep(1);
    getAccountsWiiU();

    qsort(wiiutitles, titleswiiu, sizeof(Title), titleSort);
    qsort(wiititles, titlesvwii, sizeof(Title), titleSort);

    u8* tgaBufDRC = NULL;
    u8* tgaBufTV = NULL;
    u8* fileContent = NULL;
    u32 wDRC = 0, hDRC = 0, wTV = 0, hTV = 0;

    if (loadFile("/vol/storage_sdcard/wiiu/apps/savemii_mod/backgroundDRC.tga", &fileContent) > 0) {
        wDRC = tgaGetWidth(fileContent); hDRC = tgaGetHeight(fileContent);
        tgaBufDRC = tgaRead(fileContent, TGA_READER_RGBA);
        free(fileContent);
        fileContent = NULL;
    }

    if (loadFile("/vol/storage_sdcard/wiiu/apps/savemii_mod/backgroundTV.tga", &fileContent) > 0) {
        wTV = tgaGetWidth(fileContent); hTV = tgaGetHeight(fileContent);
        tgaBufTV = tgaRead(fileContent, TGA_READER_RGBA);
        free(fileContent);
        fileContent = NULL;
    }

    while(1) {
        //u64 startTime = OSGetTime();

        if (tgaBufDRC) {
            drawBackgroundDRC(wDRC, hDRC, tgaBufDRC);
        } else {
            OSScreenClearBufferEx(1, 0x00006F00);
        }
        if (tgaBufTV) {
            drawBackgroundTV(wTV, hTV, tgaBufTV);
        } else {
            OSScreenClearBufferEx(0, 0x00006F00);
        }

        console_print_pos(0, 0, "SaveMii v%u.%u.%u.%s", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_MOD);
        console_print_pos(0, 1, "----------------------------------------------------------------------------");

        Title* titles = mode ? wiititles : wiiutitles;
        int count = mode ? titlesvwii : titleswiiu;
        int entrycount = 0;

        switch(menu) {
            case 0: { // Main Menu
                entrycount = 3;
                console_print_pos(M_OFF, 2, "   Wii U Save Management (%u Title%s)", titleswiiu, (titleswiiu > 1) ? "s": "");
                console_print_pos(M_OFF, 3, "   vWii Save Management (%u Title%s)", titlesvwii, (titlesvwii > 1) ? "s": "");
                console_print_pos(M_OFF, 4, "   Batch Backup");
                console_print_pos(M_OFF, 2 + cursor, "\u2192");
                console_print_pos_aligned(17, 4, 2, "\ue000: Select Mode");
            } break;
            case 1: { // Select Title
                if (mode == 2) {
                    entrycount = 3;
                    console_print_pos(M_OFF, 2, "   Backup All (%u Title%s)", titleswiiu + titlesvwii, ((titleswiiu + titlesvwii) > 1) ? "s": "");
                    console_print_pos(M_OFF, 3, "   Backup Wii U (%u Title%s)", titleswiiu, (titleswiiu > 1) ? "s": "");
                    console_print_pos(M_OFF, 4, "   Backup vWii (%u Title%s)", titlesvwii, (titlesvwii > 1) ? "s": "");
                    console_print_pos(M_OFF, 2 + cursor, "\u2192");
                    console_print_pos_aligned(17, 4, 2, "\ue000: Backup  \ue001: Back");
                } else {
                    console_print_pos(40, 0, "\ue084 Sort: %s %s", sortn[tsort], (tsort > 0) ? ((sorta == 1) ? "\u2193 \ue083": "\u2191 \ue083"): "");
                    entrycount = count;
                    for (int i = 0; i < 14; i++) {
                        if (i + scroll < 0 || i + scroll >= count) break;
                        ttfFontColor32(0x00FF00FF);
                        if (!titles[i + scroll].saveInit) ttfFontColor32(0xFFFF00FF);
                        if (strcmp(titles[i + scroll].shortName, "DONT TOUCH ME") == 0) ttfFontColor32(0xFF0000FF);
                        if (strlen(titles[i + scroll].shortName)) console_print_pos(M_OFF, i+2, "   %s %s%s%s", titles[i + scroll].shortName, titles[i + scroll].isTitleOnUSB ? "(USB)" : ((mode == 0) ? "(NAND)" : ""), titles[i + scroll].isTitleDupe ? " [D]" : "", titles[i + scroll].saveInit ? "" : " [Not Init]");
                        else console_print_pos(M_OFF, i+2, "   %08lx%08lx", titles[i + scroll].highID, titles[i + scroll].lowID);
                        ttfFontColor32(0xFFFFFFFF);
                        if (mode == 0) {
                            if (titles[i + scroll].iconBuf) drawTGA((M_OFF + 4) * 12 - 2, (i + 3) * 24, 0.18, titles[i + scroll].iconBuf);
                        } else if (mode == 1) {
                            if (titles[i + scroll].iconBuf) drawRGB5A3((M_OFF + 2) * 12 - 2, (i + 3) * 24 + 3, 0.25, titles[i + scroll].iconBuf);
                        }
                    }
                    if (mode == 0) {
                        console_print_pos(0, 2 + cursor, "\u2192");
                    } else if (mode == 1) {
                        console_print_pos(-1, 2 + cursor, "\u2192");
                    }
                    console_print_pos_aligned(17, 4, 2, "\ue000: Select Game  \ue001: Back");
                }
            } break;
            case 2: { // Select Task
                entrycount = 3 + 2 * (mode == 0) + 1 * ((mode == 0) && (titles[targ].isTitleDupe));
                console_print_pos(M_OFF, 2, "   [%08X-%08X] [%s]", titles[targ].highID, titles[targ].lowID, titles[targ].productCode);
                console_print_pos(M_OFF, 3, "   %s", titles[targ].shortName);
                //console_print_pos(M_OFF, 4, "   %s", titles[targ].longName);
                console_print_pos(M_OFF, 5, "   Backup savedata");
                console_print_pos(M_OFF, 6, "   Restore savedata");
                console_print_pos(M_OFF, 7, "   Wipe savedata");
                if (mode == 0) {
                    console_print_pos(M_OFF, 8, "   Import from loadiine");
                    console_print_pos(M_OFF, 9, "   Export to loadiine");
	                if (titles[targ].isTitleDupe) {
	                    console_print_pos(M_OFF, 10, "   Copy Savedata to Title in %s", titles[targ].isTitleOnUSB ? "NAND" : "USB");
	                }
                    if (titles[targ].iconBuf) drawTGA(660, 80, 1, titles[targ].iconBuf);
                } else if (mode == 1) {
                    if (titles[targ].iconBuf) drawRGB5A3(650, 80, 1, titles[targ].iconBuf);
                }
                console_print_pos(M_OFF, 2 + 3 + cursor, "\u2192");
                console_print_pos_aligned(17, 4, 2, "\ue000: Select Task  \ue001: Back");
            } break;
            case 3: { // Select Options
                entrycount = 3;
                console_print_pos(M_OFF, 2, "[%08X-%08X] %s", titles[targ].highID, titles[targ].lowID, titles[targ].shortName);

		        if (task == 5) {
                    console_print_pos(M_OFF, 4, "Destination:");
                    console_print_pos(M_OFF, 5, "    (%s)", titles[targ].isTitleOnUSB ? "NAND" : "USB");
                } else if (task > 2) {
                    entrycount = 2;
                    console_print_pos(M_OFF, 4, "Select %s:", "version");
                    console_print_pos(M_OFF, 5, "   < v%u >", versionList ? versionList[slot] : 0);
                } else if (task == 2) {
                    console_print_pos(M_OFF, 4, "Delete from:");
                    console_print_pos(M_OFF, 5, "    (%s)", titles[targ].isTitleOnUSB ? "USB" : "NAND");
                } else {
                    console_print_pos(M_OFF, 4, "Select %s:", "slot");

                    if (((titles[targ].highID & 0xFFFFFFF0) == 0x00010000) && (slot == 255)) {
                        console_print_pos(M_OFF, 5, "   < SaveGame Manager GX > (%s)", isSlotEmpty(titles[targ].highID, titles[targ].lowID, slot) ? "Empty" : "Used");
                    } else {
                        console_print_pos(M_OFF, 5, "   < %03u > (%s)", slot, isSlotEmpty(titles[targ].highID, titles[targ].lowID, slot) ? "Empty" : "Used");
                    }
                }

                if (mode == 0) {
                    if (task == 1) {
                        if (!isSlotEmpty(titles[targ].highID, titles[targ].lowID, slot)) {
                            entrycount++;
                            console_print_pos(M_OFF, 7, "Select SD user to copy from:");
                            if (sdusers == -1)
                                console_print_pos(M_OFF, 8, "   < %s >", "all users");
                            else
                                console_print_pos(M_OFF, 8, "   < %s > (%s)", sdacc[sdusers].persistentID, hasAccountSave(&titles[targ], true, false, sdacc[sdusers].pID, slot, 0) ? "Has Save" : "Empty");
                        }
                    }

                    if (task == 2) {
                        console_print_pos(M_OFF, 7, "Select Wii U user to delete from:");
                        if (allusers == -1)
                            console_print_pos(M_OFF, 8, "   < %s >", "all users");
                        else
                            console_print_pos(M_OFF, 8, "   < %s (%s) > (%s)", wiiuacc[allusers].miiName, wiiuacc[allusers].persistentID, hasAccountSave(&titles[targ], false, false, wiiuacc[allusers].pID, slot, 0) ? "Has Save" : "Empty");
                    }

                    if ((task == 0) || (task == 1) || (task == 5)) {
                        if ((task == 1) && isSlotEmpty(titles[targ].highID, titles[targ].lowID, slot)) {
                            entrycount--;
                        } else {
                            console_print_pos(M_OFF, (task == 1) ? 10 : 7, "Select Wii U user%s:", (task == 5) ? " to copy from" : ((task == 1) ? " to copy to" : ""));
                            if (allusers == -1)
                                console_print_pos(M_OFF, (task == 1) ? 11 : 8, "   < %s >", "all users");
                            else
                                console_print_pos(M_OFF, (task == 1) ? 11 : 8, "   < %s (%s) > (%s)", wiiuacc[allusers].miiName, wiiuacc[allusers].persistentID, hasAccountSave(&titles[targ], ((task == 0) || (task == 1) || (task == 5) ? false : true), ((task < 3) || (task == 5) ? false : true), wiiuacc[allusers].pID, slot, versionList ? versionList[slot] : 0) ? "Has Save" : "Empty");
                        }
                    }

                    if (task == 5) {
                        entrycount++;
                        console_print_pos(M_OFF, 10, "Select Wii U user%s:", (task == 5) ? " to copy to" : "");
                        if (allusers_d == -1)
                            console_print_pos(M_OFF, 11, "   < %s >", "all users");
                        else
                            console_print_pos(M_OFF, 11, "   < %s (%s) > (%s)", wiiuacc[allusers_d].miiName, wiiuacc[allusers_d].persistentID, hasAccountSave(&titles[titles[targ].dupeID], false, false, wiiuacc[allusers_d].pID, 0, 0) ? "Has Save" : "Empty");
                    }

                    if ((task != 3) && (task != 4)) {
                        if (allusers > -1) {
                            if (hasCommonSave(&titles[targ], ((task == 0) || (task == 2) || (task == 5) ? false : true), ((task < 3) || (task == 5) ? false : true), slot, versionList ? versionList[slot] : 0)) {
                                console_print_pos(M_OFF, (task == 1) || (task == 5) ? 13 : 10, "Include 'common' save?");
                                console_print_pos(M_OFF, (task == 1) || (task == 5) ? 14 : 11, "   < %s >", common ? "yes" : "no ");
                            } else {
                                common = false;
                                console_print_pos(M_OFF, (task == 1) || (task == 5) ? 13 : 10, "No 'common' save found.");
                                entrycount--;
                            }
                        } else {
                            common = false;
                            entrycount--;
                        }
                    } else {
                        if (hasCommonSave(&titles[targ], true, true, slot, versionList ? versionList[slot] : 0)) {
                            console_print_pos(M_OFF, 7, "Include 'common' save?");
                            console_print_pos(M_OFF, 8, "   < %s >", common ? "yes" : "no ");
                        } else {
                            common = false;
                            console_print_pos(M_OFF, 7, "No 'common' save found.");
                            entrycount--;
                        }
                    }

                    console_print_pos(M_OFF, 5 + cursor * 3, "\u2192");
                    if (titles[targ].iconBuf) drawTGA(660, 100, 1, titles[targ].iconBuf);
                } else if (mode == 1) {
                    entrycount = 1;
                    if (titles[targ].iconBuf) drawRGB5A3(650, 100, 1, titles[targ].iconBuf);
                }

                switch(task) {
                    case 0: console_print_pos_aligned(17, 4, 2, "\ue000: Backup  \ue001: Back"); break;
                    case 1: console_print_pos_aligned(17, 4, 2, "\ue000: Restore  \ue001: Back"); break;
                    case 2: console_print_pos_aligned(17, 4, 2, "\ue000: Wipe  \ue001: Back"); break;
                    case 3: console_print_pos_aligned(17, 4, 2, "\ue000: Import  \ue001: Back"); break;
                    case 4: console_print_pos_aligned(17, 4, 2, "\ue000: Export  \ue001: Back"); break;
                    case 5: console_print_pos_aligned(17, 4, 2, "\ue000: Copy  \ue001: Back"); break;
                }
            } break;
        }
        console_print_pos(0,16, "----------------------------------------------------------------------------");
        console_print_pos(0,17, "Press \ue044 to exit.");

        /*u32 passedMs = (OSGetTime() - startTime) * 4000ULL / BUS_SPEED;
        console_print_pos(-4, -1, "%d", passedMs);*/

        flipBuffers();
        while(1) {
            updatePressedButtons();
            updateHeldButtons();
            if (isPressed(PAD_BUTTON_ANY) || isHeld(PAD_BUTTON_ANY) || stickPos(4, 0.7)) break;
        }
        updatePressedButtons();
        updateHeldButtons();

        if (isPressed(PAD_BUTTON_DOWN) || isHeld(PAD_BUTTON_DOWN) || stickPos(1, -0.7) || stickPos(3, -0.7)) {
            if (entrycount <= 14) cursor = (cursor + 1) % entrycount;
            else if (cursor < 6) cursor++;
            else if ((cursor + scroll + 1) % entrycount) scroll++;
            else cursor = scroll = 0;
            os_usleep(100000);
        } else if (isPressed(PAD_BUTTON_UP) || isHeld(PAD_BUTTON_UP) || stickPos(1, 0.7) || stickPos(3, 0.7)) {
            if (scroll > 0) cursor -= (cursor>6) ? 1 : 0 * (scroll--);
            else if (cursor > 0) cursor--;
            else if (entrycount > 14) scroll = entrycount - (cursor = 6) - 1;
            else cursor = entrycount - 1;
            os_usleep(100000);
        }

        if (isPressed(PAD_BUTTON_LEFT) || isHeld(PAD_BUTTON_LEFT) || stickPos(0, -0.7) || stickPos(2, -0.7)) {
            if (menu==3) {
                if (task == 5) {
                    switch(cursor) {
                        case 0: break;
                        case 1:
                            allusers = ((allusers == -1) ? -1 : (allusers - 1));
                            allusers_d = allusers;
                            break;
                        case 2:
                            allusers_d = (((allusers == -1) || (allusers_d == -1)) ? -1 : (allusers_d - 1));
                            allusers_d = ((allusers > -1) && (allusers_d == -1)) ? 0 : allusers_d;
                            break;
                        case 3: common ^= 1; break;
                    }
                } else if (task == 1) {
                    switch(cursor) {
                        case 0: slot--; getAccountsSD(&titles[targ], slot); break;
                        case 1:
                            sdusers = ((sdusers == -1) ? -1 : (sdusers - 1));
                            allusers = ((sdusers == -1) ? -1 : allusers);
                            break;
                        case 2:
                            allusers = (((allusers == -1) || (sdusers == -1)) ? -1 : (allusers - 1));
                            allusers = ((sdusers > -1) && (allusers == -1)) ? 0 : allusers;
                            break;
                        case 3: common ^= 1; break;
                    }
                } else if (task == 2) {
                    switch(cursor) {
                        case 0: break;
                        case 1: allusers = ((allusers == -1) ? -1 : (allusers - 1)); break;
                        case 2: common ^= 1; break;
                    }
                } else if ((task == 3) || (task == 4)) {
                    switch(cursor) {
                        case 0: slot--; break;
                        case 1: common ^= 1; break;
                    }
                } else {
                    switch(cursor) {
                        case 0: slot--; break;
                        case 1: allusers = ((allusers == -1) ? -1 : (allusers - 1)); break;
                        case 2: common ^= 1; break;
                    }
                }
            }
            os_usleep(100000);
        } else if (isPressed(PAD_BUTTON_RIGHT) || isHeld(PAD_BUTTON_RIGHT) || stickPos(0, 0.7) || stickPos(2, 0.7)) {
            if (menu == 3) {
                if (task == 5) {
                    switch(cursor) {
                        case 0: break;
                        case 1:
                            allusers = ((allusers == (wiiuaccn - 1)) ? (wiiuaccn - 1) : (allusers + 1));
                            allusers_d = allusers;
                            break;
                        case 2:
                            allusers_d = ((allusers_d == (wiiuaccn - 1)) ? (wiiuaccn - 1) : (allusers_d + 1));
                            allusers_d = (allusers == -1) ? -1 : allusers_d;
                            break;
                        case 3: common ^= 1; break;
                    }
                } else if (task == 1) {
                    switch(cursor) {
                        case 0: slot++; getAccountsSD(&titles[targ], slot); break;
                        case 1:
                            sdusers = ((sdusers == (sdaccn - 1)) ? (sdaccn - 1) : (sdusers + 1));
                            allusers = ((sdusers > -1) && (allusers == -1)) ? 0 : allusers;
                            break;
                        case 2:
                            allusers = ((allusers == (wiiuaccn - 1)) ? (wiiuaccn - 1) : (allusers + 1));
                            allusers = (sdusers == -1) ? -1 : allusers;
                            break;
                        case 3: common ^= 1; break;
                    }
                } else if (task == 2) {
                    switch(cursor) {
                        case 0: break;
                        case 1: allusers = ((allusers == (wiiuaccn - 1)) ? (wiiuaccn - 1) : (allusers + 1)); break;
                        case 2: common ^= 1; break;
                    }
                }  else if ((task == 3) || (task == 4)) {
                    switch(cursor) {
                        case 0: slot++; break;
                        case 1: common ^= 1; break;
                    }
                } else {
                    switch(cursor) {
                        case 0: slot++; break;
                        case 1: allusers = ((allusers == (wiiuaccn - 1)) ? (wiiuaccn - 1) : (allusers + 1)); break;
                        case 2: common ^= 1; break;
                    }
                }
            }
            os_usleep(100000);
        }

        if (isPressed(PAD_BUTTON_R)) {
            if (menu == 1) {
                tsort = (tsort + 1) % 4;
                qsort(titles, count, sizeof(Title), titleSort);
            } else if (menu == 2) {

                targ = ++targ % count;
            }
        }

        if (isPressed(PAD_BUTTON_L)) {
            if ((menu==1) && (tsort > 0)) {
                sorta *= -1;
                qsort(titles, count, sizeof(Title), titleSort);
            } else if (menu == 2) {
                targ--;
                if (targ < 0) targ = count -1;
            }
        }

        if (isPressed(PAD_BUTTON_A)) {
            clearBuffers();
            if (menu < 3) {
                if (menu == 0) {
                    mode = cursor;
                    if (mode == 0 && (!wiiutitles || !titleswiiu)) {
                        promptError("No Wii U titles found.");
                        continue;
                    }

                    if (mode == 1 && (!wiititles || !titlesvwii)) {
                        promptError("No vWii saves found.");
                        continue;
                    }
                }

                if (menu == 1) {
                    targ = cursor + scroll;
                    cursorb = cursor;
                    scrollb = scroll;
                    if (mode == 2) {
                        OSCalendarTime dateTime;
                        switch(cursor) {
                            case 0:
                                dateTime.year = 0;
                                backupAllSave(wiiutitles, titleswiiu, &dateTime);
                                backupAllSave(wiititles, titlesvwii, &dateTime);
                                break;
                            case 1: backupAllSave(wiiutitles, titleswiiu, NULL); break;
                            case 2: backupAllSave(wiititles, titlesvwii, NULL); break;
                        }
                        continue;
                    }
                    if (titles[targ].highID==0 || titles[targ].lowID==0) continue;
                    if ((mode == 0) && (strcmp(titles[targ].shortName, "DONT TOUCH ME") == 0)) {
                        if (!promptConfirm(ST_ERROR, "CBHC save. Could be dangerous to modify. Continue?") || !promptConfirm(ST_WARNING, "Are you REALLY sure?")) {
                            cursor = cursorb;
                            scroll = scrollb;
                            continue;
                        }
                    }
                    if ((mode == 0) && (!titles[targ].saveInit)) {
                        if (!promptConfirm(ST_WARNING, "Recommended to run Game at least one time. Continue?") || !promptConfirm(ST_WARNING, "Are you REALLY sure?")) {
                            cursor = cursorb;
                            scroll = scrollb;
                            continue;
                        }
                    }
                }

                if (menu == 2) {
                    task = cursor;
                    cursort = cursor;

                    if (task == 0) {
                        if (!titles[targ].saveInit) {
                            promptError("No save to Backup.");
                            continue;
                        }
                    }

                    if (task == 1) {
                        getAccountsSD(&titles[targ], slot);
                        allusers = ((sdusers == -1) ? -1 : allusers);
                        sdusers = ((allusers == -1) ? -1 : sdusers);
                    }

                    if (task == 2) {
                        if (!titles[targ].saveInit) {
                            promptError("No save to Wipe.");
                            continue;
                        }
                    }

                    if ((task == 3) || (task == 4)) {
                        char gamePath[PATH_SIZE];
                        memset(versionList, 0, 0x100 * sizeof(int));
                        if (getLoadiineGameSaveDir(gamePath, titles[targ].productCode) != 0) continue;
                        getLoadiineSaveVersionList(versionList, gamePath);
                        if (task == 4) {
                            if (!titles[targ].saveInit) {
                                promptError("No save to Export.");
                                continue;
                            }
                        }
                    }

                    if (task == 5) {
                        if (!titles[targ].saveInit) {
                            promptError("No save to Copy.");
                            continue;
                        }
                    }
                }
                menu++;
                cursor = scroll = 0;
            } else {
                switch(task) {
                    case 0: backupSavedata(&titles[targ], slot, allusers, common); break;
                    case 1: restoreSavedata(&titles[targ], slot, sdusers, allusers, common); break;
                    case 2: wipeSavedata(&titles[targ], allusers, common); break;
                    case 3: importFromLoadiine(&titles[targ], common, versionList ? versionList[slot] : 0); break;
                    case 4: exportToLoadiine(&titles[targ], common, versionList ? versionList[slot] : 0); break;
                    case 5:
                        for (int i = 0; i < count; i++) {
                            if (titles[i].listID == titles[targ].dupeID) {
                                copySavedata(&titles[targ], &titles[i], allusers, allusers_d, common);
                                break;
                            }
                        } break;
                }
            }
        } else if (isPressed(PAD_BUTTON_B) && menu > 0) {
            clearBuffers();
            menu--;
            cursor = scroll = 0;
            if (menu == 1) {
                cursor = cursorb;
                scroll = scrollb;
            }
            if (menu == 2) cursor = cursort;
        }
        if (isPressed(PAD_BUTTON_HOME)) break;
    }

    if (tgaBufDRC) free(tgaBufDRC);
    if (tgaBufTV) free(tgaBufTV);
    freeFont(fontBuf);
    unloadTitles(wiiutitles, titleswiiu);
    unloadTitles(wiititles, titlesvwii);
    free(versionList);

    fatUnmount("sd");
    fatUnmount("usb");
    IOSUHAX_sdio_disc_interface.shutdown();
    IOSUHAX_usb_disc_interface.shutdown();

    unmount_fs("slccmpt01");
    //unmount_fs("sd");
    unmount_fs("storage_mlc");
    unmount_fs("storage_usb");
    unmount_fs("storage_odd");

    //unmount_sd_fat("sd");

    IOSUHAX_FSA_Close(fsaFd);

    if (mcp_hook_fd >= 0) MCPHookClose();
    else IOSUHAX_Close();

    return EXIT_SUCCESS;
}
