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
#ifndef __SOCKET_FUNCTIONS_H_
#define __SOCKET_FUNCTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

extern u32 nsysnet_handle;

#include <gctypes.h>

#define INADDR_ANY      0

#define AF_INET         2

#define SOCK_STREAM     1
#define SOCK_DGRAM      2

#define IPPROTO_IP      0
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

#define TCP_NODELAY     0x2004

#define SOL_SOCKET      -1
#define SO_REUSEADDR    0x0004
#define SO_NONBLOCK     0x1016
#define SO_MYADDR       0x1013
#define SO_RCVTIMEO	0x1006

#define SOL_SOCKET      -1
#define MSG_DONTWAIT    32

#define htonl(x) x
#define htons(x) x
#define ntohl(x) x
#define ntohs(x) x


struct in_addr {
    u32 s_addr;
};
struct sockaddr_in {
    short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

struct sockaddr
{
   unsigned short sa_family;
   char sa_data[14];
};


void InitSocketFunctionPointers(void);
void InitAcquireSocket(void);

extern void (*socket_lib_init)(void);
extern s32 (*socket)(s32 domain, s32 type, s32 protocol);
extern s32 (*socketclose)(s32 s);
extern s32 (*connect)(s32 s, void *addr, s32 addrlen);
extern s32 (*bind)(s32 s,struct sockaddr *name,s32 namelen);
extern s32 (*listen)(s32 s,u32 backlog);
extern s32 (*accept)(s32 s,struct sockaddr *addr,s32 *addrlen);
extern s32 (*send)(s32 s, const void *buffer, s32 size, s32 flags);
extern s32 (*recv)(s32 s, void *buffer, s32 size, s32 flags);
extern s32 (*recvfrom)(s32 sockfd, void *buf, s32 len, s32 flags,struct sockaddr *src_addr, s32 *addrlen);

extern s32 (*sendto)(s32 s, const void *buffer, s32 size, s32 flags, const struct sockaddr *dest, s32 dest_len);
extern s32 (*setsockopt)(s32 s, s32 level, s32 optname, void *optval, s32 optlen);

extern s32 (* NSSLWrite)(s32 connection, const void* buf, s32 len,s32 * written);
extern s32 (* NSSLRead)(s32 connection, const void* buf, s32 len,s32 * read);
extern s32 (* NSSLCreateConnection)(s32 context, const char* host, s32 hotlen,s32 options,s32 sock,s32 block);

extern char * (*inet_ntoa)(struct in_addr in);
extern s32 (*inet_aton)(const char *cp, struct in_addr *inp);

#ifdef __cplusplus
}
#endif

#endif // __SOCKET_FUNCTIONS_H_
