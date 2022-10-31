#pragma once
#include "ffconf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions of physical drive number for each drive */
#define DEV_SD		0
#define DEV_USB_EXT 1

#define DEV_SD_NAME "sd"
#define DEV_USB_EXT_NAME "extusb"

#define SD_PATH		"/dev/sdcard01"
#define USB_EXT1_PATH	"/dev/usb01"
#define USB_EXT2_PATH	"/dev/usb02"

extern int deviceFds[FF_VOLUMES];
extern const char *devicePaths[FF_VOLUMES];

const char *get_fat_usb_path();

#ifdef __cplusplus
}
#endif