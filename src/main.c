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

u8 slot = 0;
bool allusers = 0, common = 1;
int menu = 0, mode = 0, task = 0, targ = 0;
int cursor = 0, scroll = 0;
int cursorb = 0, scrollb = 0;
int titleswiiu = 0, titlesvwii = 0;

//just to be able to call async
void someFunc(void *arg) {
    (void)arg;
}

static int mcp_hook_fd = -1;
int MCPHookOpen() {
    //take over mcp thread
    mcp_hook_fd = MCP_Open();
    if(mcp_hook_fd < 0) return -1;
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
    if(mcp_hook_fd < 0) return;
    //close down wupserver, return control to mcp
    IOSUHAX_Close();
    //wait for mcp to return
    sleep(1);
    MCP_Close(mcp_hook_fd);
    mcp_hook_fd = -1;
}

Title* loadWiiUTitles() {

    // Source: haxchi installer
    int mcp_handle = MCP_Open();
    int count = MCP_TitleCount(mcp_handle);
    int listSize = count*0x61;
    char *tList = memalign(32, listSize);
    memset(tList, 0, listSize);
    int receivedCount = count;
    MCP_TitleList(mcp_handle, &receivedCount, tList, listSize);
    MCP_Close(mcp_handle);

    Title* titles = malloc(receivedCount*sizeof(Title));
    if (!titles) {
        promptError("Out of memory.");
        return NULL;
    }

    for (int i = 0; i < receivedCount; i++) {

        char* element = tList+(i*0x61);
        u32 highID = *(u32*)(element), lowID = *(u32*)(element+4);
        if (highID!=0x00050000) continue;
        bool isTitleOnUSB = (memcmp(element+0x56,"usb",4)==0);
        char path[255];
        memset(path, 0, 255);
        if (memcmp(element+0x56,"odd",4)==0) strcpy(path, "storage_odd:/meta/meta.xml");
        else sprintf(path, "storage_%s:/usr/title/%08x/%08x/meta/meta.xml", element+0x56, highID, lowID);

        FILE* xmlFile = fopen(path, "rb");
        if (xmlFile) {
            fseek(xmlFile, 0, SEEK_END);
            size_t xmlSize = ftell(xmlFile);
            fseek(xmlFile, 0, SEEK_SET);
            char* xmlBuf = malloc(xmlSize+1);
            if (xmlBuf) {
                memset(xmlBuf, 0, xmlSize+1);
                fread(xmlBuf, 1, xmlSize, xmlFile);
                fclose(xmlFile);

                xmlDocPtr tmp = xmlReadMemory(xmlBuf, xmlSize, "meta.xml", "utf-8", 0);
                xmlNode* root_element = xmlDocGetRootElement(tmp);
                xmlNode* cur_node = NULL;
                for (cur_node = root_element->children; cur_node; cur_node = cur_node->next) {
                    if (
                        (cur_node->type != XML_ELEMENT_NODE) ||
                        (xmlNodeGetContent(cur_node) == NULL) ||
                        (!strlen((char*)xmlNodeGetContent(cur_node)))
                    ) continue;

                    if (!memcmp(cur_node->name, "shortname_en", 13))
                        strcpy(titles[titleswiiu].shortName, (char*)xmlNodeGetContent(cur_node));
                    if (!memcmp(cur_node->name, "product_code", 13))
                        strcpy(titles[titleswiiu].productCode, (char*)(xmlNodeGetContent(cur_node)+6));
                }

                xmlFreeDoc(tmp);
                free(xmlBuf);
            }
        }

        titles[titleswiiu].highID = highID;
        titles[titleswiiu].lowID = lowID;
        titles[titleswiiu].isTitleOnUSB = isTitleOnUSB;
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

Title* loadWiiTitles() {

    struct dirent* dirent = NULL;
    DIR* dir = NULL;

    dir = opendir("slccmpt01:/title/00010000");
    if (dir == NULL) return NULL;

    while ((dirent = readdir(dir)) != 0) {
        if(strcmp(dirent->d_name, "..")==0 || strcmp(dirent->d_name, ".")==0) continue;
        titlesvwii++;
    } rewinddir(dir);

    Title* titles = malloc(titlesvwii*sizeof(Title));
    if (!titles) {
        promptError("Out of memory.");
        return NULL;
    }

    int i = 0;
    while ((dirent = readdir(dir)) != 0) {

        if(strcmp(dirent->d_name, "..")==0 || strcmp(dirent->d_name, ".")==0) continue;

        char path[256];
        sprintf(path, "slccmpt01:/title/00010000/%s/data/banner.bin", dirent->d_name);
        FILE* bnrFile = fopen(path, "rb");
        if (bnrFile) {
            fseek(bnrFile, 0x20, SEEK_SET);
            u16* bnrBuf = (u16*)malloc(0x40);
            if (bnrBuf) {
                fread(bnrBuf, 0x02, 0x20, bnrFile);
                fclose(bnrFile);
                for (int j = 0, k = 0; j < 0x20; j++) {
                    titles[i].shortName[k++] = (char)bnrBuf[j];
                }
                free(bnrBuf);
            }
        }

        titles[i].highID = 0x00010000;
        titles[i].lowID = strtoul(dirent->d_name, NULL, 16);
        i++;

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);
        console_print_pos(0, 1, "Loaded %i Wii titles.", i);
        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);

    }

    closedir(dir);
    return titles;

}

void unloadTitles(Title* titles) {
    if (titles) free(titles);
}

/* Entry point */
int Menu_Main(void) {

    mount_sd_fat("sd");

    int res = IOSUHAX_Open(NULL);
    if (res < 0) {
        res = MCPHookOpen();
    }
    if (res < 0) {
        promptError("IOSUHAX_Open failed.");
        unmount_sd_fat("sd");
        return EXIT_SUCCESS;
    }

    fatInitDefault();

    int fsaFd = IOSUHAX_FSA_Open();
    if (fsaFd < 0) {
        promptError("IOSUHAX_FSA_Open failed.");
        unmount_sd_fat("sd");
        if (mcp_hook_fd >= 0) MCPHookClose();
        else IOSUHAX_Close();
        return EXIT_SUCCESS;
    }

    mount_fs("slccmpt01", fsaFd, "/dev/slccmpt01", "/vol/storage_slccmpt01");
    mount_fs("storage_mlc", fsaFd, NULL, "/vol/storage_mlc01");
    mount_fs("storage_usb", fsaFd, NULL, "/vol/storage_usb01");
    mount_fs("storage_odd", fsaFd, "/dev/odd03", "/vol/storage_odd_content");

    ucls();
    Title* wiiutitles = loadWiiUTitles();
    Title* wiititles = loadWiiTitles();
    int* versionList = (int*)malloc(0x100*sizeof(int));

    while(1) {

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);

        console_print_pos(0, 0, "SaveMii v%u.%u.%u", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
        console_print_pos(0, 1, "--------------------------------------------------");

        Title* titles = mode ? wiititles : wiiutitles;
        int count = mode ? titlesvwii : titleswiiu;
        int entrycount = 0;

        switch(menu) {
            case 0: { // Main Menu
                entrycount = 2;
                console_print_pos(0, 2, "   Wii U Save Management");
                console_print_pos(0, 3, "   vWii Save Management");
                console_print_pos(0, 2 + cursor, "->");
            } break;
            case 1: { // Select Title
                entrycount = count;
                for (int i = 0; i < 14; i++) {
                    if (i+scroll<0 || i+scroll>=count) break;
                    if (strlen(titles[i+scroll].shortName)) console_print_pos(0, i+2, "   %s", titles[i+scroll].shortName);
                    else console_print_pos(0, i+2, "   %08lx%08lx", titles[i+scroll].highID, titles[i+scroll].lowID);
                } console_print_pos(0, 2 + cursor, "->");
            } break;
            case 2: { // Select Task
                entrycount = 3 + 2*(mode==0);
                console_print_pos(0, 2, "   Backup savedata");
                console_print_pos(0, 3, "   Restore savedata");
                console_print_pos(0, 4, "   Wipe savedata");
                if (mode==0) {
                    console_print_pos(0, 5, "   Import from loadiine");
                    console_print_pos(0, 6, "   Export to loadiine");
                }
                console_print_pos(0, 2 + cursor, "->");
            } break;
            case 3: { // Select Options
                entrycount = 1 + 2*(mode==0);
                console_print_pos(0, 2, "Select %s:", task>2 ? "version" : "slot");
                if (task > 2) console_print_pos(0, 3, "   < v%u >", versionList ? versionList[slot] : 0);
                else console_print_pos(0, 3, "   < %03u >", slot);
                if (mode==0) {
                    console_print_pos(0, 5, "Select user:");
                    console_print_pos(0, 6, "   < %s >", (allusers&&(task<3)) ? "all users" : "this user");
                    console_print_pos(0, 8, "Include 'common' save?");
                    console_print_pos(0, 9, "   < %s >", common ? "yes" : "no ");
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

        if (isPressed(VPAD_BUTTON_DOWN) || isHeld(VPAD_BUTTON_DOWN)) {
            if (entrycount<=14) cursor = (cursor + 1) % entrycount;
            else if (cursor < 6) cursor++;
            else if ((cursor+scroll+1) % entrycount) scroll++;
            else cursor = scroll = 0;
            usleep(100000);
        } else if (isPressed(VPAD_BUTTON_UP) || isHeld(VPAD_BUTTON_UP)) {
            if (scroll > 0) cursor -= (cursor>6) ? 1 : 0*(scroll--);
            else if (cursor > 0) cursor--;
            else if (entrycount>14) scroll = entrycount - (cursor = 6) - 1;
            else cursor = entrycount - 1;
            usleep(100000);
        }

        if (isPressed(VPAD_BUTTON_LEFT) || isHeld(VPAD_BUTTON_LEFT)) {
            if (menu==3) {
                switch(cursor) {
                    case 0: slot--; break;
                    case 1: allusers^=1; break;
                    case 2: common^=1; break;
                }
            }
            usleep(100000);
        } else if (isPressed(VPAD_BUTTON_RIGHT) || isHeld(VPAD_BUTTON_RIGHT)) {
            if (menu==3) {
                switch(cursor) {
                    case 0: slot++; break;
                    case 1: allusers^=1; break;
                    case 2: common^=1; break;
                }
            }
            usleep(100000);
        }

        if (isPressed(VPAD_BUTTON_A)) {
            ucls();
            if (menu<3) {
                if (menu==0) {
                    mode = cursor;
                    if (mode==0 && (!wiiutitles || !titleswiiu)) {
                        promptError("No Wii U titles found.");
                        continue;
                    }
                    if (mode==1 && (!wiititles || !titlesvwii)) {
                        promptError("No vWii saves found.");
                        continue;
                    }
                }
                if (menu==1) {
                    targ = cursor+scroll;
		            cursorb = cursor;
		            scrollb = scroll;
                    if (titles[targ].highID==0 || titles[targ].lowID==0) continue;
                }
                if (menu==2) {
                    task = cursor;
                    if (task==2) {
                        wipeSavedata(&titles[targ], allusers, common);
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
                    case 2: wipeSavedata(&titles[targ], allusers, common); break;
                    case 3: importFromLoadiine(&titles[targ], common, versionList ? versionList[slot] : 0); break;
                    case 4: exportToLoadiine(&titles[targ], common, versionList ? versionList[slot] : 0); break;
                }
            }
        } else if (isPressed(VPAD_BUTTON_B) && menu>0) {
            ucls();
            menu--;
            cursor = scroll = 0;
	        if (menu==1) {
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
    unmount_fs("storage_mlc");
    unmount_fs("storage_usb");
    unmount_fs("storage_odd");

    unmount_sd_fat("sd");

    IOSUHAX_FSA_Close(fsaFd);

    if (mcp_hook_fd >= 0) MCPHookClose();
    else IOSUHAX_Close();

    return EXIT_SUCCESS;
}
