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
#define VERSION_MINOR 0
#define VERSION_MICRO 0

u8 slot = 0;
int menu = 0, mode = 0, task = 0, targ = 0;
int cursor = 0, scroll = 0;
int titleswiiu = 0, titlesvwii = 0;

typedef struct {
    u32 highID;
    u32 lowID;
    char shortname[256];
    bool isTitleOnUSB;
} Title;

//just to be able to call async
void someFunc(void *arg)
{
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
        bool isTitleOnUSB = memcmp(element+0x56,"mlc",4)!=0;
        char path[255];
        memset(path, 0, 255);
        sprintf(path, "storage_%s:/usr/title/%08x/%08x/meta/meta.xml", isTitleOnUSB ? "usb" : "mlc", highID, lowID);

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
                        (memcmp(cur_node->name, "shortname_en", 13) != 0) ||
                        (xmlNodeGetContent(cur_node) == NULL) ||
                        (!strlen((char*)xmlNodeGetContent(cur_node)))
                    ) continue;
                    strcpy(titles[titleswiiu].shortname, (char*)xmlNodeGetContent(cur_node));
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

    struct dirent *dirent = NULL;
    DIR *dir = NULL;

    dir = opendir("slccmpt01:/title/00010000");
    if (dir == NULL) {
        promptError("Failed to open directory.");
        return NULL;
    }

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
                    titles[i].shortname[k++] = (char)bnrBuf[j];
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
    free(titles);
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
        return EXIT_SUCCESS;
    }

    fatInitDefault();

    int fsaFd = IOSUHAX_FSA_Open();
    if (fsaFd < 0) {
        promptError("IOSUHAX_FSA_Open failed.");
        if (mcp_hook_fd >= 0) MCPHookClose();
        else IOSUHAX_Close();
        return EXIT_SUCCESS;
    }

    mount_fs("slccmpt01", fsaFd, "/dev/slccmpt01", "/vol/storage_slccmpt01");
    mount_fs("storage_mlc", fsaFd, NULL, "/vol/storage_mlc01");
    mount_fs("storage_usb", fsaFd, NULL, "/vol/storage_usb01");

    ucls();
    Title* wiiutitles = loadWiiUTitles();
    Title* wiititles = loadWiiTitles();

    while(1) {

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);

        console_print_pos(0, 0, "SaveMii v%u.%u.%u", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
        console_print_pos(0, 1, "--------------------------------------------------");

        Title* titles = mode ? wiititles : wiiutitles;
        int count = mode ? titlesvwii : titleswiiu;

        switch(menu) {
            case 0: { // Main Menu
                console_print_pos(0, 2, "   Wii U Save Management");
                console_print_pos(0, 3, "   vWii Save Management");
                console_print_pos(0, 2 + cursor, "->");
            } break;
            case 1: { // WiiU/vWii Save Management
                console_print_pos(0, 2, "   Backup savedata");
                console_print_pos(0, 3, "   Restore savedata");
                // console_print_pos(0, 4, "   Wipe savedata");
                console_print_pos(0, 2 + cursor, "->");
            } break;
            case 2: { // Wii/Wii U Title List
                for (int i = 0; i < 14; i++) {
                    if (i+scroll<0 || i+scroll>=count) break;
                    if (strlen(titles[i+scroll].shortname)) console_print_pos(0, i+2, "   %s", titles[i+scroll].shortname);
                    else console_print_pos(0, i+2, "   %08lx%08lx", titles[i+scroll].highID, titles[i+scroll].lowID);
                } console_print_pos(0, 2 + cursor, "->");
            } break;
            case 3: { // Slot select
                console_print_pos(0, 2, "Press LEFT or RIGHT to select slot.");
                console_print_pos(0, 3, "              < %03u >", slot);
            } break;
        }

        console_print_pos(0,16, "--------------------------------------------------");
        console_print_pos(0,17, "Press HOME to exit.");

        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);

        updatePressedButtons();
        updateHeldButtons();

        int entrycount = ((menu==0) ? 2 : ((menu==1) ? 2 : count));

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
            if (menu==3) slot--;
            usleep(100000);
        } else if (isPressed(VPAD_BUTTON_RIGHT) || isHeld(VPAD_BUTTON_RIGHT)) {
            if (menu==3) slot++;
            usleep(100000);
        }

        if (isPressed(VPAD_BUTTON_A)) {
            ucls();
            if (menu<3) {
                if (menu==0) mode = cursor;
                if (menu==1) task = cursor;
                if (menu==2) targ = cursor+scroll;
                menu++;
                cursor = scroll = 0;
            } else {
                switch(task) {
                    case 0: backupSavedata(titles[targ].highID, titles[targ].lowID, titles[targ].isTitleOnUSB, slot); break;
                    case 1: restoreSavedata(titles[targ].highID, titles[targ].lowID, titles[targ].isTitleOnUSB, slot); break;
                    case 2: wipeSavedata(titles[targ].highID, titles[targ].lowID, titles[targ].isTitleOnUSB); break;
                }
            }
        } else if (isPressed(VPAD_BUTTON_B)) {
            ucls();
            if (menu>0) menu--;
            cursor = scroll = 0;
        }

        if (isPressed(VPAD_BUTTON_HOME)) break;

    }

    unloadTitles(wiiutitles);
    unloadTitles(wiititles);

    fatUnmount("sd");
    fatUnmount("usb");
    IOSUHAX_sdio_disc_interface.shutdown();
    IOSUHAX_usb_disc_interface.shutdown();

    unmount_fs("slccmpt01");
    unmount_fs("storage_mlc");
    unmount_fs("storage_usb");

    unmount_sd_fat("sd");

    IOSUHAX_FSA_Close(fsaFd);

    if (mcp_hook_fd >= 0) MCPHookClose();
    else IOSUHAX_Close();

    return EXIT_SUCCESS;
}
