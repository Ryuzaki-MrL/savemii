/****************************************************************************
 * Copyright (C) 2015
 * by Dimok
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 ***************************************************************************/
#include "common/common.h"
#include "os_functions.h"

u32 coreinit_handle __attribute__((section(".data"))) = 0;

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Lib handle functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_DECL(s32, OSDynLoad_Acquire, const char* rpl, u32 *handle);
EXPORT_DECL(s32, OSDynLoad_FindExport, u32 handle, s32 isdata, const char *symbol, void *address);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Security functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_DECL(s32, OSGetSecurityLevel, void);
EXPORT_DECL(s32, OSForceFullRelaunch, void);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Thread functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_DECL(s32, OSCreateThread, void *thread, s32 (*callback)(s32, void*), s32 argc, void *args, u32 stack, u32 stack_size, s32 priority, u32 attr);
EXPORT_DECL(s32, OSResumeThread, void *thread);
EXPORT_DECL(s32, OSSuspendThread, void *thread);
EXPORT_DECL(void, OSExitThread, u32 result);
EXPORT_DECL(s32, OSIsThreadTerminated, void *thread);
EXPORT_DECL(s32, OSIsThreadSuspended, void *thread);
EXPORT_DECL(s32, OSSetThreadPriority, void * thread, s32 priority);
EXPORT_DECL(s32, OSJoinThread, void * thread, s32 * ret_val);
EXPORT_DECL(void, OSDetachThread, void * thread);
EXPORT_DECL(void, OSSleepTicks, u64 ticks);
EXPORT_DECL(u64, OSGetTick, void);
EXPORT_DECL(u64, OSGetTime, void);
EXPORT_DECL(void, OSTicksToCalendarTime, u64 time, OSCalendarTime * calendarTime);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Mutex functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_DECL(void, OSInitMutex, void* mutex);
EXPORT_DECL(void, OSLockMutex, void* mutex);
EXPORT_DECL(void, OSUnlockMutex, void* mutex);
EXPORT_DECL(s32, OSTryLockMutex, void* mutex);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Shared Data functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_DECL(bool, OSGetSharedData, u32 type, u32 unk_r4, u8 *addr, u32 *size);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! System functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_DECL(u64, OSGetTitleID, void);
EXPORT_DECL(void, OSGetArgcArgv, s32* argc, char*** argv);
EXPORT_DECL(void, __Exit, void);
EXPORT_DECL(void, OSFatal, const char* msg);
EXPORT_DECL(void, OSSetExceptionCallback, u8 exceptionType, exception_callback newCallback);
EXPORT_DECL(void, DCFlushRange, const void *addr, u32 length);
EXPORT_DECL(void, DCStoreRange, const void *addr, u32 length);
EXPORT_DECL(void, ICInvalidateRange, const void *addr, u32 length);
EXPORT_DECL(void*, OSEffectiveToPhysical, const void*);
EXPORT_DECL(s32, __os_snprintf, char* s, s32 n, const char * format, ...);
EXPORT_DECL(s32 *, __gh_errno_ptr, void);

EXPORT_DECL(void, OSScreenInit, void);
EXPORT_DECL(u32, OSScreenGetBufferSizeEx, u32 bufferNum);
EXPORT_DECL(s32, OSScreenSetBufferEx, u32 bufferNum, void * addr);
EXPORT_DECL(s32, OSScreenClearBufferEx, u32 bufferNum, u32 temp);
EXPORT_DECL(s32, OSScreenFlipBuffersEx, u32 bufferNum);
EXPORT_DECL(s32, OSScreenPutFontEx, u32 bufferNum, u32 posX, u32 posY, const char * buffer);
EXPORT_DECL(s32, OSScreenEnableEx, u32 bufferNum, s32 enable);
EXPORT_DECL(u32, OSScreenPutPixelEx, u32 bufferNum, u32 posX, u32 posY, u32 color);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Memory functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_VAR(u32 *, pMEMAllocFromDefaultHeapEx);
EXPORT_VAR(u32 *, pMEMAllocFromDefaultHeap);
EXPORT_VAR(u32 *, pMEMFreeToDefaultHeap);

EXPORT_DECL(s32, MEMGetBaseHeapHandle, s32 mem_arena);
EXPORT_DECL(u32, MEMGetAllocatableSizeForFrmHeapEx, s32 heap, s32 align);
EXPORT_DECL(void *, MEMAllocFromFrmHeapEx, s32 heap, u32 size, s32 align);
EXPORT_DECL(void, MEMFreeToFrmHeap, s32 heap, s32 mode);
EXPORT_DECL(void *, MEMAllocFromExpHeapEx, s32 heap, u32 size, s32 align);
EXPORT_DECL(s32 , MEMCreateExpHeapEx, void* address, u32 size, unsigned short flags);
EXPORT_DECL(void *, MEMDestroyExpHeap, s32 heap);
EXPORT_DECL(void, MEMFreeToExpHeap, s32 heap, void* ptr);
EXPORT_DECL(void *, OSAllocFromSystem, int size, int alignment);
EXPORT_DECL(void, OSFreeToSystem, void *addr);
EXPORT_DECL(int, OSIsAddressValid, void *ptr);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! MCP functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_DECL(s32, MCP_Open, void);
EXPORT_DECL(s32, MCP_Close, s32 handle);
EXPORT_DECL(s32, MCP_TitleCount, s32 handle);
EXPORT_DECL(s32, MCP_TitleList, s32 handle, s32 *res, void *data, s32 count);
EXPORT_DECL(s32, MCP_GetOwnTitleInfo, s32 handle, void * data);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Loader functions (not real rpl)
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_DECL(s32, LiWaitIopComplete, s32 unknown_syscall_arg_r3, s32 * remaining_bytes);
EXPORT_DECL(s32, LiWaitIopCompleteWithInterrupts, s32 unknown_syscall_arg_r3, s32 * remaining_bytes);
EXPORT_DECL(void, addr_LiWaitOneChunk, void);
EXPORT_DECL(void, addr_sgIsLoadingBuffer, void);
EXPORT_DECL(void, addr_gDynloadInitialized, void);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Kernel function addresses
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_DECL(void, addr_PrepareTitle_hook, void);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Other function addresses
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
EXPORT_DECL(void, DCInvalidateRange, void *buffer, u32 length);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Energy Saver functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Burn-in Reduction
EXPORT_DECL(s32, IMEnableDim,void);
EXPORT_DECL(s32, IMDisableDim,void);
EXPORT_DECL(s32, IMIsDimEnabled,s32 * result);
//Auto power down
EXPORT_DECL(s32, IMEnableAPD,void);
EXPORT_DECL(s32, IMDisableAPD,void);
EXPORT_DECL(s32, IMIsAPDEnabled,s32 * result);
EXPORT_DECL(s32, IMIsAPDEnabledBySysSettings,s32 * result);

EXPORT_DECL(s32, OSSendAppSwitchRequest,s32 param,void* unknown1,void* unknown2);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! IOS functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

EXPORT_DECL(s32, IOS_Ioctl,s32 fd, u32 request, void *input_buffer,u32 input_buffer_len, void *output_buffer, u32 output_buffer_len);
EXPORT_DECL(s32, IOS_IoctlAsync,s32 fd, u32 request, void *input_buffer,u32 input_buffer_len, void *output_buffer, u32 output_buffer_len, void *cb, void *cbarg);
EXPORT_DECL(s32, IOS_Open,char *path, u32 mode);
EXPORT_DECL(s32, IOS_Close,s32 fd);

void _os_find_export(u32 handle, const char *funcName, void *funcPointer)
{
    OSDynLoad_FindExport(handle, 0, funcName, funcPointer);

    if(!*(u32 *)funcPointer) {
        /*
         * This is effectively OSFatal("Function %s is NULL", funcName),
         * but we can't rely on any library functions like snprintf or
         * strcpy at this point.
         *
         * Buffer bounds are not checked. Beware!
         */
        char buf[256], *bufp = buf;
        const char a[] = "Function ", b[] = " is NULL", *p;
        unsigned int i;

        for (i = 0; i < sizeof(a) - 1; i++)
            *bufp++ = a[i];

        for (p = funcName; *p; p++)
            *bufp++ = *p;

        for (i = 0; i < sizeof(b) - 1; i++)
            *bufp++ = b[i];

        *bufp++ = '\0';

        OSFatal(buf);
    }
}

void InitAcquireOS(void)
{
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Lib handle functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    EXPORT_FUNC_WRITE(OSDynLoad_Acquire, (s32 (*)(const char*, unsigned *))OS_SPECIFICS->addr_OSDynLoad_Acquire);
    EXPORT_FUNC_WRITE(OSDynLoad_FindExport, (s32 (*)(u32, s32, const char *, void *))OS_SPECIFICS->addr_OSDynLoad_FindExport);

    OSDynLoad_Acquire("coreinit.rpl", &coreinit_handle);
}

void InitOSFunctionPointers(void)
{
    u32 *funcPointer = 0;

    InitAcquireOS();

    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Security functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    OS_FIND_EXPORT(coreinit_handle, OSGetSecurityLevel);
    OS_FIND_EXPORT(coreinit_handle, OSForceFullRelaunch);
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Shared Data functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    OS_FIND_EXPORT(coreinit_handle, OSGetSharedData);
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! System functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    OS_FIND_EXPORT(coreinit_handle, OSFatal);
    OS_FIND_EXPORT(coreinit_handle, OSGetTitleID);
    OS_FIND_EXPORT(coreinit_handle, OSGetArgcArgv);
    OS_FIND_EXPORT(coreinit_handle, OSSetExceptionCallback);
    OS_FIND_EXPORT(coreinit_handle, DCFlushRange);
	OS_FIND_EXPORT(coreinit_handle, DCStoreRange);
    OS_FIND_EXPORT(coreinit_handle, ICInvalidateRange);
    OS_FIND_EXPORT(coreinit_handle, OSEffectiveToPhysical);
    OS_FIND_EXPORT(coreinit_handle, __os_snprintf);
    OS_FIND_EXPORT(coreinit_handle, __gh_errno_ptr);

    OSDynLoad_FindExport(coreinit_handle, 0, "_Exit", &__Exit);

    OS_FIND_EXPORT(coreinit_handle, OSScreenInit);
    OS_FIND_EXPORT(coreinit_handle, OSScreenGetBufferSizeEx);
    OS_FIND_EXPORT(coreinit_handle, OSScreenSetBufferEx);
    OS_FIND_EXPORT(coreinit_handle, OSScreenClearBufferEx);
    OS_FIND_EXPORT(coreinit_handle, OSScreenFlipBuffersEx);
    OS_FIND_EXPORT(coreinit_handle, OSScreenPutFontEx);
    OS_FIND_EXPORT(coreinit_handle, OSScreenEnableEx);
    OS_FIND_EXPORT(coreinit_handle, OSScreenPutPixelEx);
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Thread functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    OS_FIND_EXPORT(coreinit_handle, OSCreateThread);
    OS_FIND_EXPORT(coreinit_handle, OSResumeThread);
    OS_FIND_EXPORT(coreinit_handle, OSSuspendThread);
    OS_FIND_EXPORT(coreinit_handle, OSExitThread);
    OS_FIND_EXPORT(coreinit_handle, OSIsThreadTerminated);
    OS_FIND_EXPORT(coreinit_handle, OSIsThreadSuspended);
    OS_FIND_EXPORT(coreinit_handle, OSJoinThread);
    OS_FIND_EXPORT(coreinit_handle, OSSetThreadPriority);
    OS_FIND_EXPORT(coreinit_handle, OSDetachThread);
    OS_FIND_EXPORT(coreinit_handle, OSSleepTicks);
    OS_FIND_EXPORT(coreinit_handle, OSGetTick);
    OS_FIND_EXPORT(coreinit_handle, OSGetTime);
    OS_FIND_EXPORT(coreinit_handle, OSTicksToCalendarTime);

    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Mutex functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    OS_FIND_EXPORT(coreinit_handle, OSInitMutex);
    OS_FIND_EXPORT(coreinit_handle, OSLockMutex);
    OS_FIND_EXPORT(coreinit_handle, OSUnlockMutex);
    OS_FIND_EXPORT(coreinit_handle, OSTryLockMutex);
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! MCP functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    OS_FIND_EXPORT(coreinit_handle, MCP_Open);
    OS_FIND_EXPORT(coreinit_handle, MCP_Close);
	OS_FIND_EXPORT(coreinit_handle, MCP_TitleCount);
	OS_FIND_EXPORT(coreinit_handle, MCP_TitleList);
    OS_FIND_EXPORT(coreinit_handle, MCP_GetOwnTitleInfo);

    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Memory functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    OSDynLoad_FindExport(coreinit_handle, 1, "MEMAllocFromDefaultHeapEx", &pMEMAllocFromDefaultHeapEx);
    OSDynLoad_FindExport(coreinit_handle, 1, "MEMAllocFromDefaultHeap", &pMEMAllocFromDefaultHeap);
    OSDynLoad_FindExport(coreinit_handle, 1, "MEMFreeToDefaultHeap", &pMEMFreeToDefaultHeap);

    OS_FIND_EXPORT(coreinit_handle, MEMGetBaseHeapHandle);
    OS_FIND_EXPORT(coreinit_handle, MEMGetAllocatableSizeForFrmHeapEx);
    OS_FIND_EXPORT(coreinit_handle, MEMAllocFromFrmHeapEx);
    OS_FIND_EXPORT(coreinit_handle, MEMFreeToFrmHeap);
    OS_FIND_EXPORT(coreinit_handle, MEMAllocFromExpHeapEx);
    OS_FIND_EXPORT(coreinit_handle, MEMCreateExpHeapEx);
    OS_FIND_EXPORT(coreinit_handle, MEMDestroyExpHeap);
    OS_FIND_EXPORT(coreinit_handle, MEMFreeToExpHeap);
    OS_FIND_EXPORT(coreinit_handle, OSAllocFromSystem);
    OS_FIND_EXPORT(coreinit_handle, OSFreeToSystem);
    OS_FIND_EXPORT(coreinit_handle, OSIsAddressValid);

    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Other function addresses
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    OS_FIND_EXPORT(coreinit_handle, DCInvalidateRange);

    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Energy Saver functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //Burn-in Reduction
    OS_FIND_EXPORT(coreinit_handle, IMEnableDim);
    OS_FIND_EXPORT(coreinit_handle, IMDisableDim);
    OS_FIND_EXPORT(coreinit_handle, IMIsDimEnabled);
    //Auto power down
    OS_FIND_EXPORT(coreinit_handle, IMEnableAPD);
    OS_FIND_EXPORT(coreinit_handle, IMDisableAPD);
    OS_FIND_EXPORT(coreinit_handle, IMIsAPDEnabled);
    OS_FIND_EXPORT(coreinit_handle, IMIsAPDEnabledBySysSettings);

    OS_FIND_EXPORT(coreinit_handle, OSSendAppSwitchRequest);

    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! IOS functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    OS_FIND_EXPORT(coreinit_handle, IOS_Ioctl);
	OS_FIND_EXPORT(coreinit_handle, IOS_IoctlAsync);
    OS_FIND_EXPORT(coreinit_handle, IOS_Open);
    OS_FIND_EXPORT(coreinit_handle, IOS_Close);

    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Special non library functions
    //!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    if(OS_FIRMWARE == 550)
    {
        EXPORT_FUNC_WRITE(LiWaitIopComplete, (s32 (*)(s32, s32 *))0x01010180);
        EXPORT_FUNC_WRITE(LiWaitIopCompleteWithInterrupts, (s32 (*)(s32, s32 *))0x0101006C);
        EXPORT_FUNC_WRITE(addr_LiWaitOneChunk, (s32 (*)(s32, s32 *))0x0100080C);
        EXPORT_FUNC_WRITE(addr_PrepareTitle_hook, (s32 (*)(s32, s32 *))0xFFF184E4);

        EXPORT_FUNC_WRITE(addr_sgIsLoadingBuffer, (s32 (*)(s32, s32 *))0xEFE19E80);
        EXPORT_FUNC_WRITE(addr_gDynloadInitialized, (s32 (*)(s32, s32 *))0xEFE13DBC);
    }
    else if(OS_FIRMWARE == 532 || OS_FIRMWARE == 540)
    {
        EXPORT_FUNC_WRITE(LiWaitIopComplete, (s32 (*)(s32, s32 *))0x0100FFA4);                // loader.elf
        EXPORT_FUNC_WRITE(LiWaitIopCompleteWithInterrupts, (s32 (*)(s32, s32 *))0x0100FE90);  // loader.elf
        EXPORT_FUNC_WRITE(addr_LiWaitOneChunk, (s32 (*)(s32, s32 *))0x010007EC);              // loader.elf
        EXPORT_FUNC_WRITE(addr_PrepareTitle_hook, (s32 (*)(s32, s32 *))0xFFF18558);           // kernel.elf

        EXPORT_FUNC_WRITE(addr_sgIsLoadingBuffer, (s32 (*)(s32, s32 *))0xEFE19D00);           // loader.elf
        EXPORT_FUNC_WRITE(addr_gDynloadInitialized, (s32 (*)(s32, s32 *))0xEFE13C3C);         // loader.elf
    }
    else if(OS_FIRMWARE == 500 || OS_FIRMWARE == 510)
    {
        EXPORT_FUNC_WRITE(LiWaitIopComplete, (s32 (*)(s32, s32 *))0x0100FBC4);
        EXPORT_FUNC_WRITE(LiWaitIopCompleteWithInterrupts, (s32 (*)(s32, s32 *))0x0100FAB0);
        EXPORT_FUNC_WRITE(addr_LiWaitOneChunk, (s32 (*)(s32, s32 *))0x010007EC);
        EXPORT_FUNC_WRITE(addr_PrepareTitle_hook, (s32 (*)(s32, s32 *))0xFFF18534);

        EXPORT_FUNC_WRITE(addr_sgIsLoadingBuffer, (s32 (*)(s32, s32 *))0xEFE19D00);
        EXPORT_FUNC_WRITE(addr_gDynloadInitialized, (s32 (*)(s32, s32 *))0xEFE13C3C);
    }
    else if(OS_FIRMWARE == 410)
    {
        EXPORT_FUNC_WRITE(LiWaitIopComplete, (s32 (*)(s32, s32 *))0x0100F78C);
        EXPORT_FUNC_WRITE(LiWaitIopCompleteWithInterrupts, (s32 (*)(s32, s32 *))0x0100F678);
        EXPORT_FUNC_WRITE(addr_LiWaitOneChunk, (s32 (*)(s32, s32 *))0x010007F8);
        EXPORT_FUNC_WRITE(addr_PrepareTitle_hook, (s32 (*)(s32, s32 *))0xFFF166DC);

        EXPORT_FUNC_WRITE(addr_sgIsLoadingBuffer, (s32 (*)(s32, s32 *))0xEFE19CC0);
        EXPORT_FUNC_WRITE(addr_gDynloadInitialized, (s32 (*)(s32, s32 *))0xEFE13BFC);
    }
    else if(OS_FIRMWARE == 400) //same for 402 and 403
    {
        EXPORT_FUNC_WRITE(LiWaitIopComplete, (s32 (*)(s32, s32 *))0x0100F78C);
        EXPORT_FUNC_WRITE(LiWaitIopCompleteWithInterrupts, (s32 (*)(s32, s32 *))0x0100F678);
        EXPORT_FUNC_WRITE(addr_LiWaitOneChunk, (s32 (*)(s32, s32 *))0x010007F8);
        EXPORT_FUNC_WRITE(addr_PrepareTitle_hook, (s32 (*)(s32, s32 *))0xFFF15E70);

        EXPORT_FUNC_WRITE(addr_sgIsLoadingBuffer, (s32 (*)(s32, s32 *))0xEFE19CC0);
        EXPORT_FUNC_WRITE(addr_gDynloadInitialized, (s32 (*)(s32, s32 *))0xEFE13BFC);
    }
    else if(OS_FIRMWARE == 310)
    {
        EXPORT_FUNC_WRITE(LiWaitIopComplete, (s32 (*)(s32, s32 *))0x0100C4E4);
        EXPORT_FUNC_WRITE(LiWaitIopCompleteWithInterrupts, (s32 (*)(s32, s32 *))0x0100C3D4);
        EXPORT_FUNC_WRITE(addr_LiWaitOneChunk, (s32 (*)(s32, s32 *))0x010004D8);
        EXPORT_FUNC_WRITE(addr_PrepareTitle_hook, (s32 (*)(s32, s32 *))0xFFF15A0C);

        EXPORT_FUNC_WRITE(addr_sgIsLoadingBuffer, (s32 (*)(s32, s32 *))0xEFE19340);
        EXPORT_FUNC_WRITE(addr_gDynloadInitialized, (s32 (*)(s32, s32 *))0xEFE1329C);
    }
    else if(OS_FIRMWARE == 300)
    {
        EXPORT_FUNC_WRITE(LiWaitIopComplete, (s32 (*)(s32, s32 *))0x0100C4E4);
        EXPORT_FUNC_WRITE(LiWaitIopCompleteWithInterrupts, (s32 (*)(s32, s32 *))0x0100C3D4);
        EXPORT_FUNC_WRITE(addr_LiWaitOneChunk, (s32 (*)(s32, s32 *))0x010004D8);
        EXPORT_FUNC_WRITE(addr_PrepareTitle_hook, (s32 (*)(s32, s32 *))0xFFF15974);

        EXPORT_FUNC_WRITE(addr_sgIsLoadingBuffer, (s32 (*)(s32, s32 *))0xEFE19340);
        EXPORT_FUNC_WRITE(addr_gDynloadInitialized, (s32 (*)(s32, s32 *))0xEFE1329C);
    }
    else
    {
        OSFatal("Missing all OS specific addresses.");
    }
}
