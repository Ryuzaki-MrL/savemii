#ifndef WIIU_H
#define WIIU_H

#include <unistd.h>
#define __STDC_WANT_LIB_EXT2__ 1
#define _GNU_SOURCE
#include <stdio.h>

#include <coreinit/screen.h>
#include <coreinit/cache.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/mcp.h>
#include <coreinit/debug.h>
#include <coreinit/ios.h>
#include <coreinit/thread.h>
#include <coreinit/filesystem.h>
#include <padscore/kpad.h>
#include <padscore/wpad.h>
#include <vpad/input.h>
#include <whb/proc.h>
#include <whb/sdcard.h>

#include <malloc.h>
#include "common/common.h"

#endif // WIIU_H
