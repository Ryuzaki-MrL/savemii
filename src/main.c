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
#define VERSION_MOD "mod3"

u8 slot = 0;
bool allusers = 0, common = 1;
int menu = 0, mode = 0, task = 0, targ = 0, tsort = 0, sorta = 1;
int cursor = 0, scroll = 0;
int cursorb = 0, scrollb = 0;
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
    sleep(1);
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
    sleep(1);
    MCP_Close(mcp_hook_fd);
    mcp_hook_fd = -1;
}

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
            return 1 * sorta;
        if (((Title*)c2)->isTitleOnUSB)
            return -1 * sorta;
        return 0;
    } else if (tsort==3) {
        if (((Title*)c1)->isTitleOnUSB && !((Title*)c2)->isTitleOnUSB)
            return 1 * sorta;
        if (!((Title*)c1)->isTitleOnUSB && ((Title*)c2)->isTitleOnUSB)
            return -1 * sorta;

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

    Title* titles = malloc(receivedCount*sizeof(Title));
    if (!titles) {
        promptError("Out of memory.");
        return NULL;
    }

    for (int i = 0; i < receivedCount; i++) {
        int srcFd = -1;
        char* element = tList+(i*0x61);
        u32 highID = *(u32*)(element), lowID = *(u32*)(element+4);
        if (highID!=0x00050000) continue;
        bool isTitleOnUSB = (memcmp(element+0x56,"usb",4)==0);
        char path[255];
        memset(path, 0, 255);
        if (memcmp(element+0x56,"odd",4)==0) strcpy(path, "/vol/storage_odd_content/meta/meta.xml");
        else sprintf(path, "/vol/storage_%s01/usr/title/%08x/%08x/meta/meta.xml", element+0x56, highID, lowID);

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

        if (strcmp(data.name, "..") == 0 || strcmp(data.name, ".") == 0) continue;
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

        if (strcmp(data.name, "..") == 0 || strcmp(data.name, ".") == 0) continue;

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
        console_print_pos(0, 1, "Loaded %i Wii titles.", i);
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
int Menu_Main(void) {

    //mount_sd_fat("sd");
    loadWiiUTitles(0, -1);

    int res = IOSUHAX_Open(NULL);
    if (res < 0) {
        res = MCPHookOpen();
    }
    if (res < 0) {
        promptError("IOSUHAX_Open failed.");
        unmount_sd_fat("sd");
        return EXIT_SUCCESS;
    }

    //fatInitDefault();

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
    //mount_fs("sd", fsaFd, NULL, "/vol/storage_sdcard");
    mount_fs("slccmpt01", fsaFd, "/dev/slccmpt01", "/vol/storage_slccmpt01");
    mount_fs("storage_mlc", fsaFd, NULL, "/vol/storage_mlc01");
    mount_fs("storage_usb", fsaFd, NULL, "/vol/storage_usb01");
    mount_fs("storage_odd", fsaFd, "/dev/odd03", "/vol/storage_odd_content");

    ucls();
    Title* wiiutitles = loadWiiUTitles(1, fsaFd);
    Title* wiititles = loadWiiTitles(fsaFd);
    int* versionList = (int*)malloc(0x100*sizeof(int));

    while(1) {

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);

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
                    if (strlen(titles[i+scroll].shortName)) console_print_pos(0, i+2, "   %s %s %s", titles[i+scroll].shortName, titles[i+scroll].isTitleOnUSB ? "(USB)" : ((mode == 0) ? "(NAND)" : ""), titles[i+scroll].isTitleDupe ? "[D]" : "");
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
                    if (hasCommonSave(&titles[targ], (task == 0 ? false : true), (task < 3 ? false : true), slot, versionList ? versionList[slot] : 0)) {
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

        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);

        updatePressedButtons();
        updateHeldButtons();

        if (isPressed(VPAD_BUTTON_DOWN) || isHeld(VPAD_BUTTON_DOWN) || stickPos(1, -0.7)) {
            if (entrycount<=14) cursor = (cursor + 1) % entrycount;
            else if (cursor < 6) cursor++;
            else if ((cursor+scroll+1) % entrycount) scroll++;
            else cursor = scroll = 0;
            usleep(100000);
        } else if (isPressed(VPAD_BUTTON_UP) || isHeld(VPAD_BUTTON_UP) || stickPos(1, 0.7)) {
            if (scroll > 0) cursor -= (cursor>6) ? 1 : 0*(scroll--);
            else if (cursor > 0) cursor--;
            else if (entrycount>14) scroll = entrycount - (cursor = 6) - 1;
            else cursor = entrycount - 1;
            usleep(100000);
        }

        if (isPressed(VPAD_BUTTON_LEFT) || isHeld(VPAD_BUTTON_LEFT) || stickPos(0, -0.7)) {
            if (menu==3) {
                switch(cursor) {
                    case 0: slot--; break;
                    case 1: allusers^=1; break;
                    case 2: common^=1; break;
                }
            }
            usleep(100000);
        } else if (isPressed(VPAD_BUTTON_RIGHT) || isHeld(VPAD_BUTTON_RIGHT) || stickPos(0, 0.7)) {
            if (menu==3) {
                switch(cursor) {
                    case 0: slot++; break;
                    case 1: allusers^=1; break;
                    case 2: common^=1; break;
                }
            }
            usleep(100000);
        }

        if (isPressed(VPAD_BUTTON_R) && (menu==1)) {
            tsort = (tsort + 1) % 4;
            qsort(wiiutitles, titleswiiu, sizeof(Title), titleSort);
        }

        if (isPressed(VPAD_BUTTON_L) && (menu==1) && (tsort > 0)) {
            sorta *= -1;
            qsort(wiiutitles, titleswiiu, sizeof(Title), titleSort);
        }

        if (isPressed(VPAD_BUTTON_X) && (menu == 1) && (mode == 0)) {
            /*targ = cursor+scroll;
            if (titles[targ].highID == 0 || titles[targ].lowID == 0) continue;
            SYSCheckTitleExists((uint64_t) titles[targ].highID << 32 | titles[targ].lowID);

            if (SYSCheckTitleExists((uint64_t) titles[targ].highID << 32 | titles[targ].lowID)) {
                SYSLaunchTitle((uint64_t) titles[targ].highID << 32 | titles[targ].lowID);
                break;
            }*/
        }

        if (isPressed(VPAD_BUTTON_A)) {
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
                }
                if (menu == 2) {
                    task = cursor;
                    if (task == 2) {
                        allusers = promptConfirm("Delete save from all users?");
                        wipeSavedata(&titles[targ], allusers, hasCommonSave(&titles[targ], false, false, 0, 0));
                        continue;
                    }
                    if (task > 2) {
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
                    wipeSavedata(&titles[targ], allusers, hasCommonSave(&titles[targ], false, false, 0, 0)); break;
                    case 3: importFromLoadiine(&titles[targ], common, versionList ? versionList[slot] : 0); break;
                    case 4: exportToLoadiine(&titles[targ], common, versionList ? versionList[slot] : 0); break;
		            case 5: copySavedata(&titles[targ], &titles[titles[targ].dupeID], allusers, common); break;
                }
            }
        } else if (isPressed(VPAD_BUTTON_B) && menu > 0) {
            ucls();
            menu--;
            cursor = scroll = 0;
            if (menu == 1) {
                cursor = cursorb;
                scroll = scrollb;
            }
        }

        if (isPressed(VPAD_BUTTON_HOME)) break;

    }

    unloadTitles(wiiutitles);
    unloadTitles(wiititles);
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

    unmount_sd_fat("sd");

    IOSUHAX_FSA_Close(fsaFd);

    if (mcp_hook_fd >= 0) MCPHookClose();
    else IOSUHAX_Close();

    return EXIT_SUCCESS;
}
