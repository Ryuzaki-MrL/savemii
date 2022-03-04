#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "main.h"
#include "savemng.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 2
#define VERSION_MICRO 0
#define VERSION_MOD "mod4"

VPADStatus status;
VPADReadError error;

u8 slot = 0;
bool allusers = 0, common = 1;
int menu = 0, mode = 0, task = 0, targ = 0, tsort = 1, sorta = 1;
int cursor = 0, scroll = 0;
int cursorb = 0, scrollb = 0;
int titleswiiu = 0, titlesvwii = 0;
const char *sortn[4] = {"None", "Name", "Storage", "Storage+Name"};

int titleSort(const void *c1, const void *c2)
{
    if (tsort==0) {
        return ((Title*)c1)->listID - ((Title*)c2)->listID;
    } else if (tsort==1) {
        return strcmp(((Title*)c1)->shortName,((Title*)c2)->shortName) * sorta;
    } else if (tsort==2) {
        if (((Title*)c1)->isTitleOnUSB == ((Title*)c2)->isTitleOnUSB)
            return 0;
        if (((Title*)c1)->isTitleOnUSB)
            return -1 * sorta;
        if (((Title*)c2)->isTitleOnUSB)
            return 1 * sorta;
        return 0;
    } else if (tsort==3) {
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
    savesl = realloc(savesl, usable*sizeof(Saves));

    int dirUH, dirNH, foundCount = 0, pos = 0, tNoSave = usable;
    for(int i = 0; i <= 1; i++) {
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
    for(int i = 0; i <= 1; i++) {
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

        int ret = IOSUHAX_FSA_OpenFile(fsaFd, path, "rb", &srcFd);
        if (ret >= 0) {
            fileStat_s fStat;
        	IOSUHAX_FSA_StatFile(fsaFd, srcFd, &fStat);
            size_t xmlSize = fStat.size;

            char* xmlBuf = malloc(xmlSize+1);
            if (xmlBuf) {
                memset(xmlBuf, 0, xmlSize+1);
                IOSUHAX_FSA_ReadFile(fsaFd, xmlBuf, 0x01, xmlSize, srcFd, 0);
                IOSUHAX_FSA_CloseFile(fsaFd, srcFd);

                char *cptr = strchr(strstr(xmlBuf, "product_code"), '>') + 7;
                strncpy(titles[titleswiiu].productCode, cptr, strcspn(cptr, "<"));

                cptr = strchr(strstr(xmlBuf, "shortname_en"), '>') + 1;
                memset(titles[titleswiiu].shortName, 0, sizeof(titles[titleswiiu].shortName));
                strncpy(titles[titleswiiu].shortName, cptr, strcspn(cptr, "<"));

                free(xmlBuf);
            }
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
        titleswiiu++;

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);
        console_print_pos(0, 0, "Loaded %i Wii U titles.", titleswiiu);
        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);

    }

    free(savesl);
    free(saves);
    free(tList);
    return titles;

}

Title* loadWiiTitles(int fsaFd) {
    int dirH;

    if (IOSUHAX_FSA_OpenDir(fsaFd, "/vol/storage_slccmpt01/title/00010000", &dirH) < 0) return -1;
    while (1) {
        directoryEntry_s data;
		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
		if (ret != 0)
			break;

        titlesvwii++;
    } IOSUHAX_FSA_RewindDir(fsaFd, dirH);

    Title* titles = malloc(titlesvwii*sizeof(Title));
    if (!titles) {
        promptError("Out of memory.");
        return NULL;
    }

    int i = 0;
    while (1) {
        directoryEntry_s data;
		int ret = IOSUHAX_FSA_ReadDir(fsaFd, dirH, &data);
		if (ret != 0)
			break;

        int srcFd = -1;
        char path[256];
        sprintf(path, "/vol/storage_slccmpt01/title/00010000/%s/data/banner.bin", data.name);
        ret = IOSUHAX_FSA_OpenFile(fsaFd, path, "rb", &srcFd);
        if (ret >= 0) {
            IOSUHAX_FSA_SetFilePos(fsaFd, srcFd, 0x20);
            u16* bnrBuf = (u16*)malloc(0x40);
            if (bnrBuf) {
                IOSUHAX_FSA_ReadFile(fsaFd, bnrBuf, 0x02, 0x20, srcFd, 0);
                IOSUHAX_FSA_CloseFile(fsaFd, srcFd);

                int uni = 0;
                for (int j = 0, k = 0; j < 0x20; j++) {
                    if (bnrBuf[j] > 127) {
                        titles[i].shortName[k++] = '?';
                        uni++;
                    } else {
                        titles[i].shortName[k++] = (char)bnrBuf[j];
                        if ((char)bnrBuf[j] == 0)
                            uni++;
                    }
                }
                free(bnrBuf);
                if (uni == 0x20)
                    sprintf(titles[i].shortName, "00010000%s (No ASCII char in title)", data.name);
            }
        } else {
            sprintf(titles[i].shortName, "00010000%s (No banner.bin)", data.name);
        }

        titles[i].highID = 0x00010000;
        titles[i].lowID = strtoul(data.name, NULL, 16);

        titles[i].listID = i;
        sprintf(titles[i].productCode, "?");
        titles[i].isTitleOnUSB = false;
        titles[i].isTitleDupe = false;
        titles[i].dupeID = 0;
        i++;

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);
        console_print_pos(0, 0, "Loaded %i Wii titles.", i);
        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);
    }

    IOSUHAX_FSA_CloseDir(fsaFd, dirH);
    return titles;

}

void unloadTitles(Title* titles) {
    if (titles) free(titles);
}

/* Entry point */
int main(void) {
    uInit();
    loadWiiUTitles(0, -1);

    int res = IOSUHAX_Open(NULL);
    if (res < 0) {
        promptError("IOSUHAX_Open failed.");
        return EXIT_SUCCESS;
    }

    int fsaFd = IOSUHAX_FSA_Open();
    if (fsaFd < 0) {
        promptError("IOSUHAX_FSA_Open failed.");
        return EXIT_SUCCESS;
    }
    setFSAFD(fsaFd);

    WHBMountSdCard();
    mount_fs("slccmpt01", fsaFd, "/dev/slccmpt01", "/vol/storage_slccmpt01");
    mount_fs("storage_mlc", fsaFd, NULL, "/vol/storage_mlc01");
    mount_fs("storage_usb", fsaFd, NULL, "/vol/storage_usb01");
    mount_fs("storage_odd", fsaFd, "/dev/odd03", "/vol/storage_odd_content");
    ucls();
    Title* wiiutitles = loadWiiUTitles(1, fsaFd);
    Title* wiititles = loadWiiTitles(fsaFd);
    int* versionList = (int*)malloc(0x100*sizeof(int));
    qsort(wiiutitles, titleswiiu, sizeof(Title), titleSort);
    qsort(wiititles, titlesvwii, sizeof(Title), titleSort);
    ucls();
    while(WHBProcIsRunning()) {
        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);
        VPADRead(VPAD_CHAN_0, &status, 1, &error);
        console_print_pos(0, 0, "SaveMii v%u.%u.%u.%s", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_MOD);
        console_print_pos(0, 1, "--------------------------------------------------");
        Title* titles = mode ? wiititles : wiiutitles;
        int count = mode ? titlesvwii : titleswiiu;
        int entrycount = 0;
        switch(menu) {
            case 0: { // Main Menu
                entrycount = 2;
                console_print_pos(0, 2, "   Wii U Save Management (%u Title%s)", titleswiiu, (titleswiiu > 1) ? "s": "");
                console_print_pos(0, 3, "   vWii Save Management (%u Title%s)", titlesvwii, (titlesvwii > 1) ? "s": "");
                console_print_pos(0, 2 + cursor, "->");
            } break;
            case 1: { // Select Title
                console_print_pos(30, 0, "Sort: %s %s", sortn[tsort], (tsort > 0) ? ((sorta == 1) ? "v": "^"): "");
                entrycount = count;
                for (int i = 0; i < 14; i++) {
                    if (i+scroll<0 || i+scroll>=count) break;
                    if (strlen(titles[i+scroll].shortName)) console_print_pos(0, i+2, "   %s %s%s%s", titles[i+scroll].shortName, titles[i+scroll].isTitleOnUSB ? "(USB)" : ((mode == 0) ? "(NAND)" : ""), titles[i+scroll].isTitleDupe ? " [D]" : "", titles[i+scroll].saveInit ? "" : " [Not Init]");
                    else console_print_pos(0, i+2, "   %08lx%08lx", titles[i+scroll].highID, titles[i+scroll].lowID);
                } console_print_pos(0, 2 + cursor, "->");
            } break;
            case 2: { // Select Task
                entrycount = 3 + 2*(mode==0) + 1*((mode==0) && (titles[targ].isTitleDupe));
                console_print_pos(0, 2, "   [%08X%08X]", titles[targ].highID, titles[targ].lowID);
                console_print_pos(0, 3, "   [%s] %s", titles[targ].productCode, titles[targ].shortName);
                console_print_pos(0, 2+3, "   Backup savedata");
                console_print_pos(0, 3+3, "   Restore savedata");
                console_print_pos(0, 4+3, "   Wipe savedata");
                if (mode==0) {
                    console_print_pos(0, 5+3, "   Import from loadiine");
                    console_print_pos(0, 6+3, "   Export to loadiine");
		                if (titles[targ].isTitleDupe) {
		                    console_print_pos(0, 7+3, "   Copy Savedata to Title in %s", titles[targ].isTitleOnUSB ? "NAND" : "USB");
		                }
                }
                console_print_pos(0, 2+3 + cursor, "->");
            } break;
            case 3: { // Select Options
                entrycount = 1 + 2*(mode==0);
                console_print_pos(0, 2, "Select %s:", task>2 ? "version" : "slot");
		        if (task == 5) console_print_pos(0, 3, "   < %s >", titles[targ].isTitleOnUSB ? "NAND" : "USB");
                else if (task > 2) console_print_pos(0, 3, "   < v%u >", versionList ? versionList[slot] : 0);
                else console_print_pos(0, 3, "   < %03u > (%s)", slot, isSlotEmpty(titles[targ].highID, titles[targ].lowID, slot) ? "Empty" : "Used");
                if (mode==0) {
                    console_print_pos(0, 5, "Select user:");
                    char usrPath[16];
                    getUserID(usrPath);
                    console_print_pos(0, 6, "   < %s > %s", (allusers&&((task<3) || task==5)) ? "all users" : "this user", (allusers&&((task<3) || task==5)) ? "" : usrPath);
                    if (hasCommonSave(&titles[targ], (task == 0 ? false : true), ((task < 3) || (task == 5) ? false : true), slot, versionList ? versionList[slot] : 0)) {
                        console_print_pos(0, 8, "Include 'common' save?");
                        console_print_pos(0, 9, "   < %s >", common ? "yes" : "no ");
                    } else {
                        common = false;
                        console_print_pos(0, 8, "No 'common' save found.");
                        entrycount--;
                    }
                    console_print_pos(0, 3 + cursor*3, "->");
                }
            } break;
        }

        console_print_pos(0,16, "--------------------------------------------------");
        console_print_pos(0,17, "Press HOME to exit.");
        flipBuffers();

        if (status.trigger & (VPAD_BUTTON_DOWN | VPAD_STICK_L_EMULATION_DOWN)) {
            if (entrycount<=14) cursor = (cursor + 1) % entrycount;
            else if (cursor < 6) cursor++;
            else if ((cursor+scroll+1) % entrycount) scroll++;
            else cursor = scroll = 0;
            usleep(100000);
        } else if (status.trigger & (VPAD_BUTTON_UP | VPAD_STICK_L_EMULATION_UP)) {
            if (scroll > 0) cursor -= (cursor>6) ? 1 : 0*(scroll--);
            else if (cursor > 0) cursor--;
            else if (entrycount>14) scroll = entrycount - (cursor = 6) - 1;
            else cursor = entrycount - 1;
            usleep(100000);
        }

        if (status.trigger & (VPAD_BUTTON_LEFT | VPAD_STICK_L_EMULATION_LEFT)) {
            if (menu==3) {
                switch(cursor) {
                    case 0: slot--; break;
                    case 1: allusers^=1; break;
                    case 2: common^=1; break;
                }
            }
            usleep(100000);
        } else if (status.trigger & (VPAD_BUTTON_RIGHT | VPAD_STICK_L_EMULATION_RIGHT)) {
            if (menu==3) {
                switch(cursor) {
                    case 0: slot++; break;
                    case 1: allusers^=1; break;
                    case 2: common^=1; break;
                }
            }
            usleep(100000);
        }

        if ((status.trigger & VPAD_BUTTON_R) && (menu==1)) {
            tsort = (tsort + 1) % 4;
            qsort(titles, count, sizeof(Title), titleSort);
        }

        if ((status.trigger & VPAD_BUTTON_L) && (menu==1) && (tsort > 0)) {
            sorta *= -1;
            qsort(titles, count, sizeof(Title), titleSort);
        }

        if (status.trigger & VPAD_BUTTON_A) {
            ucls();
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
                    targ = cursor+scroll;
                    cursorb = cursor;
                    scrollb = scroll;
                    if (titles[targ].highID==0 || titles[targ].lowID==0) continue;
                    if ((mode == 0) && (strcmp(titles[targ].shortName, "DONT TOUCH ME") == 0)) {
                        if (!promptConfirm("Haxchi/CBHC save. Could be dangerous to modify. Continue?") || !promptConfirm("Are you REALLY sure?")) {
                            cursor = cursorb;
                            scroll = scrollb;
                            continue;
                        }
                    }
                    if ((mode == 0) && (!titles[targ].saveInit)) {
                        if (!promptConfirm("Recommended to run Game at least one time. Continue?") || !promptConfirm("Are you REALLY sure?")) {
                            cursor = cursorb;
                            scroll = scrollb;
                            continue;
                        }
                    }
                }
                if (menu == 2) {
                    task = cursor;
                    if (task == 0) {
                        if (!titles[targ].saveInit) {
                            promptError("No save to Backup.");
                            continue;
                        }
                    }
                    if (task == 2) {
                        allusers = promptConfirm("Delete save from all users? A-Yes B-No");
                        if (hasCommonSave(&titles[targ], false, false, 0, 0)) {
                            if (!allusers) promptConfirm("Delete common save? A-Yes B-No");
                        } else {
                            common = false;
                        }
                        wipeSavedata(&titles[targ], allusers, common); break;
                        continue;
                    }
                    if ((task == 3) || (task == 4)) {
                        char gamePath[PATH_SIZE];
                        memset(versionList, 0, 0x100*sizeof(int));
                        if (getLoadiineGameSaveDir(gamePath, titles[targ].productCode)!=0) continue;
                        getLoadiineSaveVersionList(versionList, gamePath);
                    }
                }
                menu++;
                cursor = scroll = 0;
            } else {
                switch(task) {
                    case 0: backupSavedata(&titles[targ], slot, allusers, common); break;
                    case 1: restoreSavedata(&titles[targ], slot, allusers, common); break;
                    case 2:
                    allusers = promptConfirm("Delete save from all users? A-Yes B-No");
                    if (hasCommonSave(&titles[targ], false, false, 0, 0)) {
                        if (!allusers) promptConfirm("Delete common save? A-Yes B-No");
                    } else {
                        common = false;
                    }
                    wipeSavedata(&titles[targ], allusers, common); break;
                    case 3: importFromLoadiine(&titles[targ], common, versionList ? versionList[slot] : 0); break;
                    case 4: exportToLoadiine(&titles[targ], common, versionList ? versionList[slot] : 0); break;
                    case 5: copySavedata(&titles[targ], &titles[titles[targ].dupeID], allusers, common); break;
                }
            }
        } else if ((status.trigger & VPAD_BUTTON_B) && menu > 0) {
            ucls();
            menu--;
            cursor = scroll = 0;
            if (menu == 1) {
                cursor = cursorb;
                scroll = scrollb;
            }
        }

    }

    unloadTitles(wiiutitles);
    unloadTitles(wiititles);
    free(versionList);

    WHBUnmountSdCard();
    IOSUHAX_sdio_disc_interface.shutdown();
    IOSUHAX_usb_disc_interface.shutdown();

    unmount_fs("slccmpt01");
    unmount_fs("storage_mlc");
    unmount_fs("storage_usb");
    unmount_fs("storage_odd");


    IOSUHAX_FSA_Close(fsaFd);

    OSScreenShutdown();
    WHBProcShutdown();

    return EXIT_SUCCESS;
}
