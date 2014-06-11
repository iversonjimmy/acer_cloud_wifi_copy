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

/* 
 * The keystore is an array of elements, each element containing key material
 * of fixed length (eg, 128 bits), and associated data. Longer key material is 
 * split over many elements. Elements corresponding to one key need not be 
 * contiguous, the element contains the pointer to the next element.
 * The key handle is the array index of the first element that contains 
 * material of that key.
 */
#include "core_glue.h"

#include <esc_iosctypes.h>
#include <ioslibc.h>
#include "keystore.h"
#include "bsl_defs.h"

#define CHECK_META_HANDLE(h) \
    do { \
        if ((h >= KEY_META_MAX_SIZE) || (keyMeta[h].occupied == 0)) { \
            return IOSC_ERROR_INVALID; \
        } \
    } while (0)

#define CHECK_STORE_HANDLE(h) \
    do { \
        if ((h >= KEY_STORE_MAX_SIZE) || (keyStore[h].occupied == 0)) { \
            return IOSC_ERROR_INVALID; \
        } \
    } while (0)

#define HANDLE_TYPE(type, subType) ((type)<<4 | (subType))
 
static s32 
allocFromStore(u32 size)
{
    u32 handle;
    s32 sizeAlloc = 0;
    u32 firstHandle = (u32) -1;
    u32 prevHandle = (u32) -1;
    u32 next;

    /* find space in key store */
    for (handle = 0; sizeAlloc < size && handle < KEY_STORE_MAX_SIZE; handle++) {
        if (keyStore[handle].occupied == 0) {    
            keyZeroData(handle);
            keyStore[handle].occupied = 1;
            sizeAlloc += ELEM_MATERIAL_SIZE;
            /* need to assign next pointer if this is >=second element */
            if (sizeAlloc == ELEM_MATERIAL_SIZE) {
                firstHandle = handle;
                prevHandle = handle;       
            } else {
                /* this is subsequent handle */
                keyStore[prevHandle].next = handle;
                prevHandle = handle;
            }
        }
    }

    /* if could not allocate, free the allocated blocks */
    if (sizeAlloc < size) {
        /* free the partially used space here */
        handle = firstHandle;
        do {
            keyStore[handle].occupied = 0;
            next = keyStore[handle].next;
            keyZeroData(handle);
            handle = next;
        } while ((next != 0) && (handle < KEY_STORE_MAX_SIZE));
        return IOSC_ERROR_FAIL_ALLOC;
    }
        
    return (s32) firstHandle;
}

void
keyStoreInit(void)
{
    int i;

    for (i = 0; i < KEY_META_MAX_SIZE; i++) {
        keyMeta[i].occupied = 0;
        keyMeta[i].type = 0xff; /* this will get set later */
    }
    for (i = 0; i < KEY_STORE_MAX_SIZE; i++) {
        keyStore[i].occupied = 0;
        keyStore[i].next = 0;
    }
}

IOSCError
keyInstall(u32 keyHandle, int size)
{
    IOSCError err = IOSC_ERROR_OK;

    if ((keyHandle >= KEY_STORE_MAX_SIZE) || keyMeta[keyHandle].occupied) {
        err = IOSC_ERROR_EXISTS;
        goto out;
    }

    /* handle case where no keyStore data is needed */
    if (size == 0) {
        keyMeta[keyHandle].keyStoreHandle = 0xffff;
    } else {
        s32 storeHandle = allocFromStore(size);

        if (storeHandle < 0) {
            err = IOSC_ERROR_FAIL_ALLOC;
            goto out;
        }

        keyMeta[keyHandle].keyStoreHandle = storeHandle;
    }

    keyMeta[keyHandle].occupied = 1;
 out:
    return err;
}

IOSCError
keyDelete(u32 handle)
{
    u32 next;

    keyMeta[handle].occupied = 0;
    handle = keyMeta[handle].keyStoreHandle;
    if (handle == 0xffff) { /* no keyStore data */
        return IOSC_ERROR_OK;
    }

    do {
        CHECK_STORE_HANDLE(handle);
        keyStore[handle].occupied = 0;
        next = keyStore[handle].next;
        keyZeroData(handle);
        handle = next;
    } while ((next != 0) && (handle < KEY_STORE_MAX_SIZE));
    
    return IOSC_ERROR_OK;
}

IOSCError
keyAlloc(u32 *keyHandle, int size)
{
    int i;
    IOSCError err = IOSC_ERROR_FAIL_ALLOC;

    /* find first handle in metadata */
    for (i = BSL_MAX_DEFAULT_HANDLES + 1; i < KEY_META_MAX_SIZE; i++) {
        if (keyMeta[i].occupied == 0) {
            err = keyInstall(i, size);
            if (err == 0) {
                *keyHandle = i;
            }
            break;
        }
    }

    return err;
}

IOSCError
keyFree(u32 handle)
{
    CHECK_META_HANDLE(handle);
    return keyDelete(handle);
}

/*
 * set ownership 
 */
IOSCError 
keyInsertOwnership(u32 handle, u32 owner)
{
    CHECK_META_HANDLE(handle);
    keyMeta[handle].owner = owner;
    return IOSC_ERROR_OK;
}

/*
 * set protection bits
 */
IOSCError 
keyInsertProt(u32 handle, u32 prot)
{
    CHECK_META_HANDLE(handle);
    keyMeta[handle].prot = prot;
    return IOSC_ERROR_OK;
}

IOSCError 
keyInsertMiscData(u32 handle, u8 *material)
{
    CHECK_META_HANDLE(handle);
    memcpy(keyMeta[handle].miscData, (u8 *)material, MISC_DATA_SIZE);
    return IOSC_ERROR_OK;
}

IOSCError 
keyInsertData(u32 handle, u8 *material, u32 size)
{
    int i = 0;
    int sizeinserted = 0;
    int blocksize = 0;

    CHECK_META_HANDLE(handle);
    handle = keyMeta[handle].keyStoreHandle;
    /* fill up data */
    do {
        if ((size - sizeinserted) > ELEM_MATERIAL_SIZE) {
            blocksize = ELEM_MATERIAL_SIZE;
        } else {
            blocksize = size - sizeinserted;
        }
        memcpy(keyStore[handle].material, (u8 *)(material + (i * ELEM_MATERIAL_SIZE)), blocksize);
        handle = keyStore[handle].next;
        sizeinserted += blocksize;
        i++;
    } while (handle != 0);

    return IOSC_ERROR_OK;
}

/*
 * get data in handle: offset is multiple of ELEM_MATERIAL_SIZE, size is 
 * number of bytes from offset.
 */

IOSCError
keyGetMiscData(u32 handle, u8 *material)
{
    CHECK_META_HANDLE(handle);
    memcpy((u8 *)(material), keyMeta[handle].miscData, MISC_DATA_SIZE);
    return IOSC_ERROR_OK;
}

IOSCError 
keyGetOwnership(u32 handle, u32 *owner)
{
    CHECK_META_HANDLE(handle);
    *owner = keyMeta[handle].owner;
    return IOSC_ERROR_OK;
}

IOSCError 
keyGetProt(u32 handle, u32 *prot)
{
    CHECK_META_HANDLE(handle);
    *prot = keyMeta[handle].prot;
    return IOSC_ERROR_OK;
}

IOSCError
keyGetData(u32 handle, u8 *material, u32 size)
{
    int i = 0;
    int sizegiven = 0;
    int blocksize = 0;

    CHECK_META_HANDLE(handle);
    handle = keyMeta[handle].keyStoreHandle;
    CHECK_STORE_HANDLE(handle);
    do {
        if ((size - sizegiven) > ELEM_MATERIAL_SIZE) {
            blocksize = ELEM_MATERIAL_SIZE;
        } else {
            blocksize = size - sizegiven;
        }
        memcpy((u8 *)(material + (i*ELEM_MATERIAL_SIZE)), keyStore[handle].material, blocksize);
        handle = keyStore[handle].next;
        CHECK_STORE_HANDLE(handle);
        sizegiven += blocksize;
        i++;
    } while (handle != 0);

    return IOSC_ERROR_OK;
}

IOSCError
keySetType(u32 handle, u8 type)
{
    CHECK_META_HANDLE(handle);
    keyMeta[handle].type = type;
    return IOSC_ERROR_OK;
}

IOSCError
keyGetType(u32 handle, u8 *outtype)
{
    CHECK_META_HANDLE(handle);
    *outtype = keyMeta[handle].type;
    return IOSC_ERROR_OK;
}

void 
keyZeroData(u32 handle)
{
    memset(&(keyStore[handle]), 0x0, sizeof(keyStoreElem));
}
