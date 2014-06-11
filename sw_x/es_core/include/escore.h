/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */
#ifndef __ESCORE_H__
#define __ESCORE_H__

#include "vplu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESCORE_ERR_UNSPECIFIED   -17000
#define ESCORE_ERR_ALREADY_INIT  -17011
#define ESCORE_ERR_MUTEX         -17014
#define ESCORE_ERR_UNKNOWN_INIT_MODE -17015
#define ESCORE_ERR_ESC_DEVICE    -17016

#define ESCORE_INIT_MODE_NORMAL		0
#define ESCORE_INIT_MODE_INSECURE_CRED	1

#define ESCORE_CERT_MAX       10

#define ESCORE_MATCH_ANY      (~0)

#define ESCORE_OWNER_OS       0x00000001
#define ESCORE_OWNER_VSD      0x00000002
#define ESCORE_OWNER_CCD      0x00000003

#define ESCORE_BLOCK_PAGE_MAX	(8)

#define ESCORE_NO_HANDLE		0

struct ESCore_Ticket_s {
    u64         titleId;
    u64         userId;
    u32         handle;
    u32         cookie;
    u32         refCnt;
    u32         state;
};
typedef struct ESCore_Ticket_s ESCore_Ticket_t;

struct ESCore_Title_s {
    u64         id;
    u32         cookie;
    u32         handle;
    u32         state;
    u32         ticketHandle;
};
typedef struct ESCore_Title_s ESCore_Title_t;

struct ESCore_Content_s {
    u32         titleHandle;
    u32         id;
    u32         handle;
    u32         state;
};
typedef struct ESCore_Content_s ESCore_Content_t;

struct ESCore_Chal_s {
    u64         id;
    u32         cookie;
    u32         handle;
};
typedef struct ESCore_Chal_s ESCore_Chal;

typedef u8 ESCore_EccSignedCert[384];

int ESCore_ImportTicketTitle(u64 tid, u32 cookie, const void *tktBuf, u32 tktLen,
                             const void *certBufs[], u32 certLens[], u32 certCnt,
                             u32 *ticketHandle);

int ESCore_ImportTicketUseOnce(u64 tid, u32 cookie, const void *tktBuf, u32 tktLen,
                               const void *certBufs[], u32 certLens[], u32 certCnt,
                               u32 chalHandle, u32 *ticketHandle);

int ESCore_ImportTitle(u64 tid, u32 cookie, u32 tktSigHandle, u32 tktDataHandle,
                       const void *tmdBuf, u32 tmdLen, u32 *titleHandle);

int ESCore_ImportContentById(u32 titleHandle, u32 contentId,
                             const void *contentBuf, u32 contentLen,
                             u32 *contentHandle);
int ESCore_ImportContentByIdx(u32 titleHandle, u32 contentIndex,
                              const void *contentBuf, u32 contentLen,
                              u32 *contentHandle);

int ESCore_ReleaseContent(u32 contentHandle);

int ESCore_ReleaseTitle(u32 titleHandle);

int ESCore_ReleaseTicket(u32 ticketHandle);

int ESCore_GetDeviceId(u32 *devId);

int ESCore_GetDeviceCert(ESCore_EccSignedCert* devCert);

int ESCore_GetDeviceType(u32 *devType);

int ESCore_GetDeviceGuid(u64 *devGuid);

int ESCore_QueryTitleMax(u32 *titleMax); 
int ESCore_QueryTitle(u64 tid, u32 cookie, u32 mask, ESCore_Title_t *titles, u32 *titleCnt); 

int ESCore_QueryContentMax(u32 *contentMax); 
int ESCore_QueryContent(u32 titleHandle, ESCore_Content_t *contents, u32 *contentCnt); 

int ESCore_QueryTicketMax(u32 *ticketMax); 
int ESCore_QueryTicket(u64 titleId, u32 cookie, u32 mask, ESCore_Ticket_t *tickets,
                       u32 *ticketCnt); 

int ESCore_DecryptBlock(u32 contentHandle, u32 blockNum, u32 pageCnt,
                        void *pagesIn[ESCORE_BLOCK_PAGE_MAX],
                        void *pagesOut[ESCORE_BLOCK_PAGE_MAX]);

int ESCore_DecryptContentById(u32 tmdHandle, u32 contentId, void *bufIn, void *bufOut, u32 size);
int ESCore_DecryptContentByIdx(u32 tmdHandle, u32 contentIndex, void *bufIn, void *bufOut, u32 size);

#if defined(_GSS_SUPPORT)
int ESCore_ImportTicketUser(u64 uid, u32 cookie, const void *tktBuf, u32 tktLen,
                            const void *certBufs[], u32 certLens[], u32 certCnt,
                            u32 *ticketHandle);

int ESCore_EncryptBlock(u32 contentHandle, u32 blockNum, u32 pageCnt,
                        void *pagesIn[ESCORE_BLOCK_PAGE_MAX],
                        void *pagesOut[ESCORE_BLOCK_PAGE_MAX]);


int ESCore_CreateGss(u64 userId, u32 cookie, u32 ticketSignHandle,
                     u32 ticketDataHandle, u32 titleHandle, u32 contentId,
                     u32 *retTitleHandlep, u32 *retContentHandlep);

int ESCore_ExportTitle(u32 titleHandle, u32 ticketHandle, void *tmdBuf,
                       u32 tmdLen);

int ESCore_ExportContent(u32 contentHandle, void *contentBuf, u32 contentLen);

int ESCore_GetTitleSize(u32 titleHandle, u32 *tmdLen);

int ESCore_GetContentSize(u32 contentHandle, u32 *contentLen);

#endif // _GSS_SUPPORT

/// In the user-land configuration, the @a credSecret and @a credClear buffers are not copied,
/// so they must remain valid while ESCore is in use.
int ESCore_Init(void *credDefaul, u32 defaultLen);
int ESCore_LoadCredentials(void *credSecret, u32 secretLen, void *credClear, u32 clearLen, u32 mode);

#ifdef __cplusplus
}
#endif

#endif // __ESCORE_H__
