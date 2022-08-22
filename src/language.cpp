#include "language.h"

Swkbd_LanguageType sysLang;

void loadLanguage(Swkbd_LanguageType language) {
	switch(language) {
        /*case Swkbd_LanguageType__Japanese:
			gettextLoadLanguage("romfs:/japanese.lang");
            break;*/
		case Swkbd_LanguageType__English:
			gettextLoadLanguage("romfs:/english.lang");
            break;
		/*case Swkbd_LanguageType__French:
			gettextLoadLanguage("romfs:/french.lang");
            break;
		case Swkbd_LanguageType__German:
			gettextLoadLanguage("romfs:/german.lang");
            break;
		case Swkbd_LanguageType__Italian:
			gettextLoadLanguage("romfs:/italian.lang");
            break;*/
		case Swkbd_LanguageType__Spanish:
			gettextLoadLanguage("romfs:/spanish.lang");
            break;
		/*case Swkbd_LanguageType__Chinese1:
			gettextLoadLanguage("romfs:/chinese1.lang");
            break;
		case Swkbd_LanguageType__Korean:
			gettextLoadLanguage("romfs:/korean.lang");
            break;
		case Swkbd_LanguageType__Dutch:
			gettextLoadLanguage("romfs:/dutch.lang");
            break;
		case Swkbd_LanguageType__Potuguese:
			gettextLoadLanguage("romfs:/portuguese.lang");
            break;
		case Swkbd_LanguageType__Russian:
			gettextLoadLanguage("romfs:/russian.lang");
            break;
		case Swkbd_LanguageType__Chinese2:
			gettextLoadLanguage("romfs:/chinese2.lang");
            break;*/
		default:
			gettextLoadLanguage("romfs:/english.lang");
            break;
	}
}

Swkbd_LanguageType getSystemLanguage() {
   UCHandle handle = UCOpen();
   if(handle < 1)
      sysLang = Swkbd_LanguageType__English;
    
   UCSysConfig *settings = (UCSysConfig*)MEMAllocFromDefaultHeapEx(sizeof(UCSysConfig), 0x40);
   if(settings == nullptr) {
      UCClose(handle);
      sysLang = Swkbd_LanguageType__English;
   }

   strcpy(settings->name, "cafe.language");
   settings->access = 0;
   settings->dataType = UC_DATATYPE_UNSIGNED_INT;
   settings->error = UC_ERROR_OK;
   settings->dataSize = sizeof(Swkbd_LanguageType);
   settings->data = &sysLang;

   UCError err = UCReadSysConfig(handle, 1, settings);
   UCClose(handle);
   MEMFreeToDefaultHeap(settings);
   if(err != UC_ERROR_OK)
      sysLang = Swkbd_LanguageType__English;
   return sysLang;
}