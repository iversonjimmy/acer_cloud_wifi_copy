/*
 *               Copyright (C) 2005, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#ifndef __KEYSTORE_H__
#define __KEYSTORE_H__

#include <esc_iosctypes.h>

#include "keystore_defs.h"

#define ELEM_MATERIAL_SIZE 32
#define MISC_DATA_SIZE 4

typedef struct {
    u8 occupied;
    u8 type;
    u32 owner;
    u32 prot;
    u8 miscData[MISC_DATA_SIZE]; 
    s16 keyStoreHandle;
} keyMetaElem;

typedef struct keyData {
    u8 occupied;
    u8 material[ELEM_MATERIAL_SIZE];
    u16 next;
} keyStoreElem;

void keyStoreInit(void);
IOSCError keyInstall(u32 handle, int size);
IOSCError keyDelete(u32 handle);
IOSCError keyAlloc(u32 *handle, int size);
IOSCError keyFree(u32 handle);
IOSCError keyInsertData(u32 handle, u8 *material, u32 size);
IOSCError keyInsertMiscData(u32 handle, u8 * material);
IOSCError keyInsertOwnership(u32 handle, u32 owner);
IOSCError keyInsertProt(u32 handle, u32 prot);
IOSCError keyGetMiscData(u32 handle, u8 *material);
IOSCError keyGetOwnership(u32 handle, u32 *owner);
IOSCError keyGetProt(u32 handle, u32 *prot);
IOSCError keyGetData(u32 handle, u8 *material, u32 size);
void keyZeroData(u32 handle);
IOSCError keySetType(u32 handle, u8 type);
IOSCError keyGetType(u32 handle, u8 *outtype);
u8 keyGetKeyStoreOccupancy(u32 handle);
s32 keyGetKeyStoreSize(void);
u32 keyGetNextHandle(u32 handle);
u8 keyGetKeyMetaOccupancy(u32 handle);

extern keyMetaElem keyMeta[KEY_META_MAX_SIZE];
extern keyStoreElem keyStore[KEY_STORE_MAX_SIZE];

#endif
