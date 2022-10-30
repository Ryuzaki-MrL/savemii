#include "devices.h"
#include "ffconf.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int deviceFds[FF_VOLUMES] = {-1, -1};
const char *devicePaths[FF_VOLUMES] = {NULL, NULL};

const char *get_fat_usb_path() {
    return devicePaths[DEV_USB_EXT];
}

#ifdef __cplusplus
}
#endif