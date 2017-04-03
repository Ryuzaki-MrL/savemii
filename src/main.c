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
int titlecount = 0;

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

Title* loadTitles() {

    int mcp_handle = MCP_Open();
    int count = MCP_TitleCount(mcp_handle);
    int listSize = count*0x61;
    char *tList = memalign(32, listSize);
    memset(tList, 0, listSize);
    int receivedCount = count;
    MCP_TitleList(mcp_handle, &receivedCount, tList, listSize);
    MCP_Close(mcp_handle);

    Title* titles = malloc(receivedCount*sizeof(Title));

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
                strcpy(titles[titlecount].shortname, (char*)xmlNodeGetContent(cur_node));
            }

            xmlFreeDoc(tmp);
            free(xmlBuf);
        }

        titles[titlecount].highID = highID;
        titles[titlecount].lowID = lowID;
        titles[titlecount].isTitleOnUSB = isTitleOnUSB;
        titlecount++;

    }

    free(tList);
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
    }

    mount_fs("slccmpt01", fsaFd, "/dev/slccmpt01", "/vol/storage_slccmpt01");
    mount_fs("storage_mlc", fsaFd, NULL, "/vol/storage_mlc01");
    mount_fs("storage_usb", fsaFd, NULL, "/vol/storage_usb01");

    ucls();
    Title* titles = loadTitles();

    while(1) {

        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);

        console_print_pos(0, 0, "SaveMii v%u.%u.%u", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
        console_print_pos(0, 1, "--------------------------------------------------");

        switch(menu) {
            case 0: { // Main Menu
                console_print_pos(0, 2, "   Wii U Save Management");
                // console_print_pos(0, 3, "   vWii Save Management");
                console_print_pos(0, 2 + cursor, "->");
            } break;
            case 1: { // WiiU/vWii Save Management
                console_print_pos(0, 2, "   Backup savedata");
                console_print_pos(0, 3, "   Restore savedata");
                // console_print_pos(0, 4, "   Wipe savedata");
                console_print_pos(0, 2 + cursor, "->");
            } break;
            case 2: { // vWii/Wii U Title List
                for (int i = 0; i < 14; i++) {
                    if (i+scroll<0 || i+scroll>=titlecount) break;
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

        int entrycount = ((menu==0) ? 1 : ((menu==1) ? 2 : titlecount));

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

    unloadTitles(titles);

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
