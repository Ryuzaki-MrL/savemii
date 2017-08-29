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
#ifndef __SYSHID_FUNCTIONS_H_
#define __SYSHID_FUNCTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>

extern u32 syshid_handle;

typedef struct
{
    u32 handle;
    u32 physical_device_inst;
    u16 vid;
    u16 pid;
    u8 interface_index;
    u8 sub_class;
    u8 protocol;

    u16 max_packet_size_rx;
    u16 max_packet_size_tx;

} HIDDevice;

typedef struct _HIDClient HIDClient;

#define HID_DEVICE_DETACH   0
#define HID_DEVICE_ATTACH   1

typedef s32 (*HIDAttachCallback)(HIDClient *p_hc,HIDDevice *p_hd,u32 attach);

struct _HIDClient
{
    HIDClient *next;
    HIDAttachCallback attach_cb;
};

typedef void (*HIDCallback)(u32 handle,s32 error,u8 *p_buffer,u32 bytes_transferred,void *p_user);

void InitSysHIDFunctionPointers(void);
void InitAcquireSysHID(void);

extern s32(*HIDSetup)(void);
extern s32(*HIDTeardown)(void);

extern s32(*HIDAddClient)(HIDClient *p_client, HIDAttachCallback attach_callback);
extern s32(*HIDDelClient)(HIDClient *p_client);

extern s32(*HIDGetDescriptor)(u32 handle,u8 descriptor_type,u8 descriptor_index, u16 language_id, u8 *p_buffer, u32 buffer_length, HIDCallback hc, void *p_user);
extern s32(*HIDSetDescriptor)(u32 handle,u8 descriptor_type,u8 descriptor_index, u16 language_id, u8 *p_buffer, u32 buffer_length, HIDCallback hc, void *p_user);

extern s32(*HIDGetReport)(u32 handle, u8 report_type, u8 report_id, u8 *p_buffer, u32 buffer_length, HIDCallback hc, void *p_user);
extern s32(*HIDSetReport)(u32 handle, u8 report_type, u8 report_id, u8 *p_buffer, u32 buffer_length, HIDCallback hc, void *p_user);

extern s32(*HIDSetIdle)(u32 handle, u8 s32erface_index,u8 duration, HIDCallback hc, void *p_user);

extern s32(* HIDSetProtocol)(u32 handle,u8 s32erface_index,u8 protocol, HIDCallback hc, void *p_user);
extern s32(* HIDGetProtocol)(u32 handle,u8 s32erface_index,u8 * protocol, HIDCallback hc, void *p_user);

extern s32(*HIDRead)(u32 handle, u8 *p_buffer, u32 buffer_length, HIDCallback hc, void *p_user);
extern s32(*HIDWrite)(u32 handle, u8 *p_buffer, u32 buffer_length, HIDCallback hc, void *p_user);

#ifdef __cplusplus
}
#endif

#endif // __SYSHID_FUNCTIONS_H_
