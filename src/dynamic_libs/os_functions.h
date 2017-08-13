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
#ifndef __OS_FUNCTIONS_H_
#define __OS_FUNCTIONS_H_

#include <gctypes.h>
#include "common/os_defs.h"
#include "os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUS_SPEED                       248625000
#define SECS_TO_TICKS(sec)              (((unsigned long long)(sec)) * (BUS_SPEED/4))
#define MILLISECS_TO_TICKS(msec)        (SECS_TO_TICKS(msec) / 1000)
#define MICROSECS_TO_TICKS(usec)        (SECS_TO_TICKS(usec) / 1000000)

//To avoid conflicts with the unistd.h
#define os_usleep(usecs)                OSSleepTicks(MICROSECS_TO_TICKS(usecs))
#define os_sleep(secs)                  OSSleepTicks(SECS_TO_TICKS(secs))

#define FLUSH_DATA_BLOCK(addr)          asm volatile("dcbf 0, %0; sync" : : "r"(((addr) & ~31)))
#define INVAL_DATA_BLOCK(addr)          asm volatile("dcbi 0, %0; sync" : : "r"(((addr) & ~31)))

#define EXPORT_DECL(res, func, ...)     res (* func)(__VA_ARGS__) __attribute__((section(".data"))) = 0;
#define EXPORT_VAR(type, var)           type var __attribute__((section(".data")));


#define EXPORT_FUNC_WRITE(func, val)    *(u32*)(((u32)&func) + 0) = (u32)val

#define OS_FIND_EXPORT(handle, func)    _os_find_export(handle, # func, &funcPointer);                                  \
                                        EXPORT_FUNC_WRITE(func, funcPointer);

#define OS_FIND_EXPORT_EX(handle, func, func_p)                                                                         \
                                        _os_find_export(handle, # func, &funcPointer);                                  \
                                        EXPORT_FUNC_WRITE(func_p, funcPointer);

#define OS_MUTEX_SIZE                   44

/* Handle for coreinit */
extern u32 coreinit_handle;
extern void _os_find_export(u32 handle, const char *funcName, void *funcPointer);
extern void InitAcquireOS(void);
extern void InitOSFunctionPointers(void);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Lib handle functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern s32 (* OSDynLoad_Acquire)(const char* rpl, u32 *handle);
extern s32 (* OSDynLoad_FindExport)(u32 handle, s32 isdata, const char *symbol, void *address);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Security functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern s32 (* OSGetSecurityLevel)(void);
extern s32 (* OSForceFullRelaunch)(void);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Thread functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern s32 (* OSCreateThread)(void *thread, s32 (*callback)(s32, void*), s32 argc, void *args, u32 stack, u32 stack_size, s32 priority, u32 attr);
extern s32 (* OSResumeThread)(void *thread);
extern s32 (* OSSuspendThread)(void *thread);
extern void (*OSExitThread)(u32 result);
extern s32 (* OSIsThreadTerminated)(void *thread);
extern s32 (* OSIsThreadSuspended)(void *thread);
extern s32 (* OSJoinThread)(void * thread, s32 * ret_val);
extern s32 (* OSSetThreadPriority)(void * thread, s32 priority);
extern void (* OSDetachThread)(void * thread);
extern void (* OSSleepTicks)(u64 ticks);
extern u64 (* OSGetTick)(void);
extern u64 (* OSGetTime)(void);
extern void (*OSTicksToCalendarTime)(u64 time, OSCalendarTime *calendarTime);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Mutex functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern void (* OSInitMutex)(void* mutex);
extern void (* OSLockMutex)(void* mutex);
extern void (* OSUnlockMutex)(void* mutex);
extern s32 (* OSTryLockMutex)(void* mutex);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Shared Data functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern bool (* OSGetSharedData)(u32 type, u32 unk_r4, u8 *addr, u32 *size);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! System functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern u64 (* OSGetTitleID)(void);
extern void (* OSGetArgcArgv)(s32* argc, char*** argv);
extern void (* __Exit)(void);
extern void (* OSFatal)(const char* msg);
extern void (* DCFlushRange)(const void *addr, u32 length);
extern void (* DCStoreRange)(const void *addr, u32 length);
extern void (* ICInvalidateRange)(const void *addr, u32 length);
extern void* (* OSEffectiveToPhysical)(const void*);
extern s32 (* __os_snprintf)(char* s, s32 n, const char * format, ...);
extern s32 * (* __gh_errno_ptr)(void);

extern void (*OSScreenInit)(void);
extern u32 (*OSScreenGetBufferSizeEx)(u32 bufferNum);
extern s32 (*OSScreenSetBufferEx)(u32 bufferNum, void * addr);
extern s32 (*OSScreenClearBufferEx)(u32 bufferNum, u32 temp);
extern s32 (*OSScreenFlipBuffersEx)(u32 bufferNum);
extern s32 (*OSScreenPutFontEx)(u32 bufferNum, u32 posX, u32 posY, const char * buffer);
extern s32 (*OSScreenEnableEx)(u32 bufferNum, s32 enable);
extern u32 (*OSScreenPutPixelEx)(u32 bufferNum, u32 posX, u32 posY, u32 color);

typedef unsigned char (*exception_callback)(void * interruptedContext);
extern void (* OSSetExceptionCallback)(u8 exceptionType, exception_callback newCallback);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Memory functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern u32 *pMEMAllocFromDefaultHeapEx;
extern u32 *pMEMAllocFromDefaultHeap;
extern u32 *pMEMFreeToDefaultHeap;

extern s32 (* MEMGetBaseHeapHandle)(s32 mem_arena);
extern u32 (* MEMGetAllocatableSizeForFrmHeapEx)(s32 heap, s32 align);
extern void* (* MEMAllocFromFrmHeapEx)(s32 heap, u32 size, s32 align);
extern void (* MEMFreeToFrmHeap)(s32 heap, s32 mode);
extern void *(* MEMAllocFromExpHeapEx)(s32 heap, u32 size, s32 align);
extern s32 (* MEMCreateExpHeapEx)(void* address, u32 size, unsigned short flags);
extern void *(* MEMDestroyExpHeap)(s32 heap);
extern void (* MEMFreeToExpHeap)(s32 heap, void* ptr);
extern void* (* OSAllocFromSystem)(int size, int alignment);
extern void (* OSFreeToSystem)(void *addr);
extern int (* OSIsAddressValid)(void *ptr);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! MCP functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern s32 (* MCP_Open)(void);
extern s32 (* MCP_Close)(s32 handle);
extern s32 (* MCP_TitleCount)(s32 handle);
extern s32 (* MCP_TitleList)(s32 handle, s32 *res, void *data, s32 count);
extern s32 (* MCP_GetOwnTitleInfo)(s32 handle, void * data);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! LOADER functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern s32 (* LiWaitIopComplete)(s32 unknown_syscall_arg_r3, s32 * remaining_bytes);
extern s32 (* LiWaitIopCompleteWithInterrupts)(s32 unknown_syscall_arg_r3, s32 * remaining_bytes);
extern void (* addr_LiWaitOneChunk)(void);
extern void (* addr_sgIsLoadingBuffer)(void);
extern void (* addr_gDynloadInitialized)(void);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Kernel function addresses
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern void (* addr_PrepareTitle_hook)(void);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Other function addresses
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern void (*DCInvalidateRange)(void *buffer, u32 length);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! Energy Saver functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
////Burn-in Reduction
extern s32 (*IMEnableDim)(void);
extern s32 (*IMDisableDim)(void);
extern s32 (*IMIsDimEnabled)(s32 * result);
//Auto power down
extern s32 (*IMEnableAPD)(void);
extern s32 (*IMDisableAPD)(void);
extern s32 (*IMIsAPDEnabled)(s32 * result);
extern s32 (*IMIsAPDEnabledBySysSettings)(s32 * result);

extern s32 (*OSSendAppSwitchRequest)(s32 param,void* unknown1,void* unknown2);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! IOS functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

extern s32 (*IOS_Ioctl)(s32 fd, u32 request, void *input_buffer,u32 input_buffer_len, void *output_buffer, u32 output_buffer_len);
extern s32 (*IOS_IoctlAsync)(s32 fd, u32 request, void *input_buffer,u32 input_buffer_len, void *output_buffer, u32 output_buffer_len, void *cb, void *cbarg);
extern s32 (*IOS_Open)(char *path, u32 mode);
extern s32 (*IOS_Close)(s32 fd);

#ifdef __cplusplus
}
#endif

#endif // __OS_FUNCTIONS_H_
