#pragma once

#include "gettext.h"
#include <romfs-wiiu.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/time.h>
#include <coreinit/userconfig.h>

#include <cstring>

typedef enum
{
   Swkbd_LanguageType__Japanese  = 0,
   Swkbd_LanguageType__English   = 1,
   Swkbd_LanguageType__French    = 2,
   Swkbd_LanguageType__German    = 3,
   Swkbd_LanguageType__Italian   = 4,
   Swkbd_LanguageType__Spanish   = 5,
   Swkbd_LanguageType__Chinese1  = 6,
   Swkbd_LanguageType__Korean    = 7,
   Swkbd_LanguageType__Dutch     = 8,
   Swkbd_LanguageType__Potuguese = 9,
   Swkbd_LanguageType__Russian   = 10,
   Swkbd_LanguageType__Chinese2  = 11,
   Swkbd_LanguageType__Invalid   = 12
} Swkbd_LanguageType;

void loadLanguage(Swkbd_LanguageType language) __attribute__((cold));
Swkbd_LanguageType getSystemLanguage() __attribute__((cold));