#include "draw.h"
#include <configMenu.h>
#include <cstring>
#include <input.h>
#include <language.h>
#include <savemng.h>
#include <state.h>

#define M_OFF 1

static int cursorPos = 0;
bool redraw = true;

static char *language = (char *) "";

static void drawConfigMenuFrame() {
    clearBuffersEx();
    consolePrintPos(0, 0, "SaveMii v%u.%u.%u", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
    consolePrintPos(0, 1, "----------------------------------------------------------------------------");
    consolePrintPos(M_OFF, 2, gettext("   Language: %s"), language);
    consolePrintPos(M_OFF, 2 + cursorPos, "\u2192");
    consolePrintPos(0, 16, "----------------------------------------------------------------------------");
    consolePrintPos(0, 17, gettext("Press \ue044 to exit."));
    flipBuffers();
    WHBLogFreetypeDraw();
}

void configMenu() {
    Input input;
    getLoadedLanguage(language);
    while (AppRunning()) {
        input.read();
        if (input.get(TRIGGER, PAD_BUTTON_ANY))
            redraw = true;
        if (input.get(TRIGGER, PAD_BUTTON_B))
            break;
        if (input.get(TRIGGER, PAD_BUTTON_A) || input.get(TRIGGER, PAD_BUTTON_RIGHT)) {
            if (strcmp(language, "Japanese") == 0)
                loadLanguage(Swkbd_LanguageType__Italian);
            else if (strcmp(language, "Italian") == 0)
                loadLanguage(Swkbd_LanguageType__Spanish);
            else if (strcmp(language, "Spanish") == 0)
                loadLanguage(Swkbd_LanguageType__Chinese1);
            else if (strcmp(language, "Traditional Chinese") == 0)
                loadLanguage(Swkbd_LanguageType__Korean);
            else if (strcmp(language, "Korean") == 0)
                loadLanguage(Swkbd_LanguageType__Russian);
            else if (strcmp(language, "Russian") == 0)
                loadLanguage(Swkbd_LanguageType__Chinese2);
            else if (strcmp(language, "Simplified Chinese") == 0)
                loadLanguage(Swkbd_LanguageType__English);
            else if (strcmp(language, "English") == 0)
                loadLanguage(Swkbd_LanguageType__Japanese);
        }
        if (redraw) {
            getLoadedLanguage(language);
            drawConfigMenuFrame();
            redraw = false;
        }
    }
    return;
}