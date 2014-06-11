/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#define _XOPEN_SOURCE 600 /* for pread, pwrite */
#define _GNU_SOURCE
#include <vplu_types.h>

#ifndef IOS
#include <malloc.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <log.h>

#include "vpl_th.h"
#include "vplex_file.h"
#include <assert.h>

#include "contentfile.h"

#define USE_O_DIRECT 0

#ifdef WIN32
#if USE_O_DIRECT
#error WIN32 cannot support USE_O_DIRECT
#endif
#endif

struct contentfile
{
    VPLFile_handle_t blkfd;
    VPLFile_handle_t mapfd;     // contain valid map
    u32 blocksize;
    u32 numBlocks;
    u32 numValidBlocks;

    VPLMutex_t mutex;  /* locks data-structures */
    u8* valid_bitmap;
    u32 bitmap_size_bytes;

    // These variables are needed to ensure that a reset does not leave contentfile
    // in an inconsistent state if done during a block write.
    int num_map_writes;
    int num_blk_writes;

    int b_autosync;
};

// Map Header format: http://intwww/wiki/index.php/VSD_Content_Prefetching#Bitmap_Content_storage
struct contentfile_map_header
{
    u32 version;
    u32 blocksize;
    u32 numBlocks;
};

#define BLOCKS_TO_BITMAP_BYTES(numBlocks) (((numBlocks)+7)/8)

#define CONTENTFILE_ERR                    -1
#define CONTENTFILE_CORRUPT_STATE_ERR      -2
#define CONTENTFILE_CREATE_FILE_ERR        -3
#define CONTENTFILE_HEADER_CORRUPT_ERR     -4
#define CONTENTFILE_UNSUPPORTED_BLOCKSIZE  -5
#define CONTENTFILE_MEMORY_ERR             -6

const char mapSuffix[]="map";
const char blkSuffix[]="blk";

static char*
contentfile_alloc_blk_file_name(const char *name)
{
    char* blk_name;
    int blk_len;

    blk_len = snprintf(NULL, 0, "%s.%s", name, blkSuffix) + 1;
    blk_name = (char*)malloc(blk_len);
    if(blk_name == NULL) {
        LOG_ERROR("Out of memory");
        return NULL;
    }
    snprintf(blk_name, blk_len, "%s.%s", name, blkSuffix);
    return blk_name;
}

static char*
contentfile_alloc_map_file_name(const char *name)
{
    char* map_name;
    int map_len;

    map_len = snprintf(NULL, 0, "%s.%s", name, mapSuffix) + 1;
    map_name = (char*)malloc(map_len);
    if(map_name == NULL) {
        LOG_ERROR("Out of memory: contentfile %s", name);
        return NULL;
    }
    snprintf(map_name, map_len, "%s.%s", name, mapSuffix);
    return map_name;
}

static int
contentfile_check_blocksize(u32 blocksize)
{
    switch (blocksize) {
    case ( 1 << 10): // 1024
    case ( 1 << 12): // 4096
    case ( 1 << 15): // 32768
        return 0;
    default:
        return -1;
    }
}

static int
contentfile_reset_map_unlocked(struct contentfile* handle,
                               int mapfd,
                               struct contentfile_map_header* mapHeader)
{
    int numBytesWritten;

    numBytesWritten = VPLFile_WriteAt(mapfd, mapHeader, sizeof(*mapHeader), 0);
    if(numBytesWritten != sizeof(*mapHeader)){
        LOG_ERROR("Only wrote %d bytes for header", numBytesWritten);
        goto fail;
    }
    handle->numValidBlocks = 0;
    memset(handle->valid_bitmap, 0, handle->bitmap_size_bytes);
    numBytesWritten = VPLFile_WriteAt(mapfd,
                                      handle->valid_bitmap,
                                      handle->bitmap_size_bytes,
                                      sizeof(*mapHeader));
    if(numBytesWritten != handle->bitmap_size_bytes) {
        LOG_ERROR("Only wrote %d bytes out of %d bytes valid_bitmap",
                  numBytesWritten, handle->bitmap_size_bytes);
        goto fail;
    }
    VPLFile_Sync(mapfd);
    return 0;
    
 fail:
    return -1;
}

static struct contentfile *
contentfile_open_helper(const char *name, u32 blocksize, u32 numBlocks, s32 *error_out)
{
    char* map_name = NULL;
    VPLFile_handle_t mapfd = 0;
    char* blk_name = NULL;
    VPLFile_handle_t blkfd = 0;
    int fileExist_flags = VPLFILE_OPENFLAG_READWRITE;
    struct contentfile * handle;
    int b_justCreated = 0;
    struct contentfile_map_header mapHeader;

    *error_out = 0;

    // Compose file names.
    map_name = contentfile_alloc_map_file_name(name);
    blk_name = contentfile_alloc_blk_file_name(name);
    if(blk_name == NULL || map_name == NULL) {
        LOG_ERROR("Failed to allocate file names to open contentfile %s.", name);
        goto fail_make_names;
    }

    // 1. Open files
    mapfd = VPLFile_Open(map_name, fileExist_flags, 0600);
    blkfd = VPLFile_Open(blk_name, fileExist_flags 
#if USE_O_DIRECT
                         | VPLFILE_OPENFLAG_DIRECT
#endif
                         , 0600);

    if(!VPLFile_IsValidHandle(mapfd) && !VPLFile_IsValidHandle(blkfd)) {  // No files exist so create new files.
        int create_flags = (VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_CREATE | fileExist_flags);
        b_justCreated = 1;

        mapfd = VPLFile_Open(map_name, create_flags, 0600);
        blkfd = VPLFile_Open(blk_name, create_flags 
#if USE_O_DIRECT
                             | VPLFILE_OPENFLAG_DIRECT
#endif
                             , 0600);
    } else if (!VPLFile_IsValidHandle(mapfd) || !VPLFile_IsValidHandle(blkfd)) {
        *error_out = CONTENTFILE_CORRUPT_STATE_ERR;
        LOG_ERROR("Read corrupt contentfile state, file missing. file:%s (mapfd:%d,blkfd:%d",
                  name, mapfd, blkfd);
    }

    if(!VPLFile_IsValidHandle(mapfd) || !VPLFile_IsValidHandle(blkfd)) {
        *error_out = CONTENTFILE_CREATE_FILE_ERR;
        LOG_ERROR("Create file error. file:%s (mapfd:%d,blkfd:%d)",
                  name, mapfd, blkfd);
        goto fail;
    }

    // 2. Initialize in-memory mapHeader
    if(!b_justCreated) {  // Previously existing, read header information
        int bytesRead;
        bytesRead = VPLFile_ReadAt(mapfd, &mapHeader, sizeof mapHeader, 0);
        if(bytesRead >= sizeof mapHeader) {
            if(blocksize != mapHeader.blocksize){
                *error_out = CONTENTFILE_HEADER_CORRUPT_ERR;
                LOG_ERROR("Input blocksize does not match existing block size: existing:%d, input:%d",
                          mapHeader.blocksize, blocksize);
                goto fail;
            }
            if(CONTENTFILE_VERSION != mapHeader.version) {
                *error_out = CONTENTFILE_HEADER_CORRUPT_ERR;
                LOG_ERROR("Expected sparsefile version different: file:%d, current:%d",
                          mapHeader.version, CONTENTFILE_VERSION);
            }
        }else{
            *error_out = CONTENTFILE_HEADER_CORRUPT_ERR;
            LOG_ERROR("Header missing or corrupt: %d bytes read. Overwriting.", bytesRead);
            b_justCreated = 1;
        }
    }

    if(b_justCreated) {
        mapHeader.version = CONTENTFILE_VERSION;
        mapHeader.blocksize = blocksize;
        mapHeader.numBlocks = numBlocks;
    }

    if(contentfile_check_blocksize(mapHeader.blocksize) != 0){
        *error_out = CONTENTFILE_UNSUPPORTED_BLOCKSIZE;
        LOG_ERROR("Unsupported blocksize. file:%s blocksize:%d", name, blocksize);
        goto fail;
    }

    // 3. Initialize handle
    handle = malloc(sizeof *handle);
    if(handle==NULL){
        *error_out = CONTENTFILE_MEMORY_ERR;
        goto handle_malloc_fail;
    }
    handle->mapfd = mapfd;
    handle->blkfd = blkfd;
    handle->blocksize = mapHeader.blocksize;

    VPLMutex_Init(&handle->mutex);

    handle->numBlocks = mapHeader.numBlocks;
    handle->valid_bitmap = malloc( BLOCKS_TO_BITMAP_BYTES(mapHeader.numBlocks));
    if(handle->valid_bitmap == NULL){
        *error_out = CONTENTFILE_MEMORY_ERR;
        goto handle_bitmap_malloc_fail;
    }
    handle->bitmap_size_bytes = BLOCKS_TO_BITMAP_BYTES(mapHeader.numBlocks);
    handle->numValidBlocks = 0;

    handle->num_map_writes = 0;
    handle->num_blk_writes = 0;

    handle->b_autosync = 1;

    // 4. Initialize disk files and/or in-memory valid_bitmap
    if(b_justCreated) {
        int rc = contentfile_reset_map_unlocked(handle, handle->mapfd, &mapHeader);
        if(rc != 0) {
            LOG_ERROR("Initialize map failed");
            goto initialize_map_fail;
        }
    }else{
        u32 numBytesRead;
        u32 blockIndex;
        numBytesRead = VPLFile_ReadAt(handle->mapfd,
                                      handle->valid_bitmap,
                                      BLOCKS_TO_BITMAP_BYTES(mapHeader.numBlocks),
                                      sizeof(mapHeader));
        if(numBytesRead != BLOCKS_TO_BITMAP_BYTES(mapHeader.numBlocks)) {
            LOG_ERROR("Read of %d bytes unsuccessful.  Only %d bytes read.",
                      BLOCKS_TO_BITMAP_BYTES(mapHeader.numBlocks), numBytesRead);
            goto pwrite_pread_fail;
        }
        // Count blocks for numValidBlocks
        for(blockIndex = 0; blockIndex < mapHeader.numBlocks; blockIndex++) {
            int rc = 0;
            int valid = 0;
            rc = contentfile_get_valid_bit(handle, blockIndex, &valid);
            if(rc < 0) { LOG_ERROR("Can never happen"); }
            if(valid) {
                handle->numValidBlocks++;
            }
        }
    }
    if(blk_name) free(blk_name);
    if(map_name) free(map_name);
    return handle;

 pwrite_pread_fail:
 initialize_map_fail:
    free(handle->valid_bitmap);
    VPLMutex_Destroy(&handle->mutex);
 handle_bitmap_malloc_fail:
    free(handle);
 handle_malloc_fail:
 fail:
    VPLFile_Close(mapfd);
    VPLFile_Close(blkfd);
 fail_make_names:
    if(blk_name) free(blk_name);
    if(map_name) free(map_name);
    return NULL;
}

contentfile_handle_t
contentfile_open(const char *name, u32 blocksize, u32 numBlocks)
{
    s32 error;
    contentfile_handle_t toReturn = contentfile_open_helper(name, blocksize, numBlocks, &error);
    return toReturn;
}

int
contentfile_reset(struct contentfile* handle,
                  const char *name,
                  u32 blocksize,
                  u32 numBlocks)
{
    char* map_name = NULL;
    char* blk_name = NULL;
    int fileExist_flags = VPLFILE_OPENFLAG_READWRITE;
    int create_flags;
    int rv = 0;
    VPLFile_handle_t mapfd = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t blkfd = VPLFILE_INVALID_HANDLE;
    create_flags = (VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_CREATE | fileExist_flags);

    if(contentfile_check_blocksize(blocksize) != 0){
        LOG_ERROR("Unsupported blocksize. file:%s blocksize:%d", name, blocksize);
        rv = CONTENTFILE_UNSUPPORTED_BLOCKSIZE;
        goto fail_blocksize;
    }

    // Compose file names.
    map_name = contentfile_alloc_map_file_name(name);
    blk_name = contentfile_alloc_blk_file_name(name);
    if(blk_name == NULL || map_name == NULL) {
        LOG_ERROR("Failed to allocate file names to reset contentfile %s.", name);
        rv = CONTENTFILE_MEMORY_ERR;
        goto fail_make_names;
    }

    VPLMutex_Lock(&handle->mutex);
    VPLFile_Close(handle->mapfd);
    VPLFile_Close(handle->blkfd);
    contentfile_delete(name);

    mapfd = VPLFile_Open(map_name, create_flags, 0600);
    blkfd = VPLFile_Open(blk_name, create_flags 
#if USE_O_DIRECT
                         | VPLFILE_OPENFLAG_DIRECT
#endif
                         , 0600);

    if(!VPLFile_IsValidHandle(mapfd) || !VPLFile_IsValidHandle(blkfd)) {
        rv = CONTENTFILE_CREATE_FILE_ERR;
        LOG_ERROR("Create file error. file:%s (mapfd:%d,blkfd:%d)",
                  name, mapfd, blkfd);
        goto fail_open;
    }

    {
        struct contentfile_map_header mapHeader;
        u8* realloc_bitmap;

        realloc_bitmap = realloc(handle->valid_bitmap, BLOCKS_TO_BITMAP_BYTES(numBlocks));
        if(realloc_bitmap == NULL){
            rv = CONTENTFILE_MEMORY_ERR;
            LOG_ERROR("Failed to reallocate bitmap to reset contentfile %s.", name);
            goto handle_bitmap_malloc_fail;
        }
        handle->valid_bitmap = realloc_bitmap;
        handle->bitmap_size_bytes = BLOCKS_TO_BITMAP_BYTES(numBlocks);

        mapHeader.version = CONTENTFILE_VERSION;
        mapHeader.blocksize = blocksize;
        mapHeader.numBlocks = numBlocks;

        {
            int rc = contentfile_reset_map_unlocked(handle, mapfd, &mapHeader);
            if(rc != 0) {
                LOG_ERROR("Initialize map failed");
                rv = CONTENTFILE_ERR;
                goto initialize_map_fail;
            }
        }
    }

    handle->mapfd = mapfd;
    handle->blkfd = blkfd;
    handle->blocksize = blocksize;
    handle->numBlocks = numBlocks;

    // Variables to make sure contentfile_write does not cause a bad state where
    // mapfd file block is valid, but blkfd file is not, since mapfd and blkfd were changed.
    handle->num_map_writes = 0;
    handle->num_blk_writes = 0;

    // b_autosync variable will NOT be reset.

    VPLMutex_Unlock(&handle->mutex);
    if(map_name) { free(map_name); }
    if(blk_name) { free(blk_name); }
    return rv;
    
 initialize_map_fail:
 handle_bitmap_malloc_fail:
 fail_open:
    VPLFile_Close(mapfd);
    VPLFile_Close(blkfd);
    VPLMutex_Unlock(&handle->mutex);
 fail_make_names:
    if(map_name) { free(map_name); }
    if(blk_name) { free(blk_name); }
 fail_blocksize:
    return rv;
}


static int
contentfile_get_valid_bit_unlocked(contentfile_handle_t handle, u32 blockNum, int* valid)
{
    u32 index;
    u32 offset;
    u8 mask;
    u8 value;

    if(handle->numBlocks <= blockNum){  // Out of range
        goto fail;
    }

    index = blockNum/8;
    offset = blockNum%8;
    mask = 1;
    mask = (mask << offset);

    value = handle->valid_bitmap[index] & mask;

    if(mask == value){
        *valid = 1;
    } else {
        *valid = 0;
    }

    return 0;
fail:
    *valid = 0;
    return -1;
}

int
contentfile_resize_total_blocks(struct contentfile* handle,
                                u32 newNumBlocks)
{
    u32 origNumBlocks = handle->numBlocks;
    u32 origBitmapSizeBytes = handle->bitmap_size_bytes;
    u32 origNumValidBlocks = handle->numValidBlocks;
    struct contentfile_map_header mapHeader;
    u32 numBytesWritten;

    LOG_INFO("Resizing contentfile from %d to %d.", origNumBlocks, newNumBlocks);

    if(newNumBlocks == origNumBlocks) {
        goto done;
    }

    VPLMutex_Lock(&handle->mutex);

    if(newNumBlocks > origNumBlocks) {
        u8* resizedBitmap;
        resizedBitmap = realloc(handle->valid_bitmap, BLOCKS_TO_BITMAP_BYTES(newNumBlocks));
        if(resizedBitmap == NULL) {
            LOG_ERROR("Unable to reallocate %d bytes.", BLOCKS_TO_BITMAP_BYTES(newNumBlocks));
            goto fail_realloc;
        }
        handle->valid_bitmap = resizedBitmap;

        // initialize newly allocated portion of the bitmap and write to file
        memset(resizedBitmap+BLOCKS_TO_BITMAP_BYTES(origNumBlocks),
               0,
               BLOCKS_TO_BITMAP_BYTES(newNumBlocks)-
                   BLOCKS_TO_BITMAP_BYTES(origNumBlocks));

        numBytesWritten = VPLFile_WriteAt(handle->mapfd,
                                          &handle->valid_bitmap[BLOCKS_TO_BITMAP_BYTES(origNumBlocks)],
                                          BLOCKS_TO_BITMAP_BYTES(newNumBlocks)-
                                          BLOCKS_TO_BITMAP_BYTES(origNumBlocks),
                                          sizeof(struct contentfile_map_header)+
                                          BLOCKS_TO_BITMAP_BYTES(origNumBlocks));
        if(numBytesWritten != (BLOCKS_TO_BITMAP_BYTES(newNumBlocks)-
                                BLOCKS_TO_BITMAP_BYTES(origNumBlocks))) {
            LOG_ERROR("Only wrote %d bytes for mapBody when %d expected",
                      numBytesWritten,
                      BLOCKS_TO_BITMAP_BYTES(newNumBlocks)-
                          BLOCKS_TO_BITMAP_BYTES(origNumBlocks));;
            goto fail_write_map_body;
        }
    }
    else if(newNumBlocks < origNumBlocks) {  // Simply truncate the data structure
        u32 numValidDecrease = 0;
        u32 index;
        for(index = 0; index < origNumBlocks - newNumBlocks; index++) {
            int rc;
            int valid;
            // count number valid decreases
            rc = contentfile_get_valid_bit_unlocked(handle, index+newNumBlocks, &valid);
            if(rc < 0) {
                LOG_ERROR("contentfile_get_valid_bit, blockNum:%d, rc:%d",
                          index+origNumBlocks, rc);
                goto fail_get_valid_bit;
            }
            if(valid) {
                numValidDecrease++;
            }
        }

        handle->numValidBlocks -= numValidDecrease;
    }

    handle->numBlocks = newNumBlocks;
    handle->bitmap_size_bytes = BLOCKS_TO_BITMAP_BYTES(newNumBlocks);

    // Update mapfile header.
    mapHeader.blocksize = handle->blocksize;
    mapHeader.numBlocks = newNumBlocks;
    mapHeader.version = CONTENTFILE_VERSION;
    numBytesWritten = VPLFile_WriteAt(handle->mapfd, &mapHeader, sizeof(mapHeader), 0);
    if(numBytesWritten != sizeof(mapHeader)){
        LOG_ERROR("Only wrote %d bytes for header. Expecting "FMTu_size_t,
                  numBytesWritten, sizeof(mapHeader));
        goto fail_write_map_header;
    }
    VPLMutex_Unlock(&handle->mutex);

    VPLFile_Sync(handle->mapfd);

 done:
    return 0;

 fail_write_map_header:
 fail_get_valid_bit:
 fail_write_map_body:
 fail_realloc:
    // Revert back to original values to leave contentfile in a good state
    handle->numBlocks = origNumBlocks;
    handle->bitmap_size_bytes = origBitmapSizeBytes;
    handle->numValidBlocks = origNumValidBlocks;

    VPLMutex_Unlock(&handle->mutex);
    return -1;
}

int
contentfile_get_autosync(struct contentfile* handle)
{
    return handle->b_autosync;
}

void
contentfile_set_autosync(struct contentfile* handle, int b_autosync)
{
    handle->b_autosync = b_autosync;
}

void
contentfile_sync(struct contentfile* handle)
{
    VPLFile_Sync(handle->blkfd);
    VPLFile_Sync(handle->mapfd);
}

int
contentfile_close(struct contentfile* handle)
{
    contentfile_sync(handle);
    VPLFile_Close(handle->blkfd);
    VPLFile_Close(handle->mapfd);

    VPLMutex_Destroy(&handle->mutex);

    free(handle->valid_bitmap);
    free(handle);
    return 0;
}

int
contentfile_get_valid_bit(contentfile_handle_t handle, u32 blockNum, int* valid)
{
    int rc;

    VPLMutex_Lock(&handle->mutex);
    rc = contentfile_get_valid_bit_unlocked(handle, blockNum, valid);
    VPLMutex_Unlock(&handle->mutex);

    return rc;
}

static int
contentfile_set_valid_bit(contentfile_handle_t handle, u32 blockNum)
{
    u32 index;
    u32 offset;
    u8 mask;
    int fail_rv = CONTENTFILE_ERR;

    VPLMutex_Lock(&handle->mutex);
    if(handle->numBlocks <= blockNum){  // Out of range
        goto fail;
    }

    handle->num_map_writes++;
    if(handle->num_map_writes > handle->num_blk_writes) {
        struct contentfile_map_header mapHeader;
        LOG_ERROR("Contentfile skipped blk write during reset: "
                  "map_writes:%d, blk_writes:%d, blockNum:%d",
                  handle->num_map_writes, handle->num_blk_writes, blockNum);

        mapHeader.blocksize = handle->blocksize;
        mapHeader.version = CONTENTFILE_VERSION;
        mapHeader.numBlocks = handle->numBlocks;
        contentfile_reset_map_unlocked(handle, handle->mapfd, &mapHeader);
        handle->num_map_writes = 0;
        handle->num_blk_writes = 0;
        fail_rv = CONTENTFILE_CORRUPT_STATE_ERR;
        goto fail;
    }

    // Error checking done, setting the bit
    index = blockNum/8;
    offset = blockNum%8;
    mask = 1;
    mask = (mask << offset);

    if((handle->valid_bitmap[index] & (u8)mask) == 0) {  // Bit actually needs setting
        ssize_t numwrote;
        handle->numValidBlocks++;
        handle->valid_bitmap[index] = handle->valid_bitmap[index] | (u8)mask;
        // TODO check return value?
        numwrote = VPLFile_WriteAt(handle->mapfd, &handle->valid_bitmap[index], 1,
                                           sizeof(struct contentfile_map_header)+index);
        (void)numwrote;
    }
    VPLMutex_Unlock(&handle->mutex);
    return 0;

fail:
    VPLMutex_Unlock(&handle->mutex);
    return fail_rv;
}

static int
contentfile_clear_valid_bit(contentfile_handle_t handle, u32 blockNum)
{
    u32 index;
    u32 offset;
    u32 mask;
    u32 mask_inv;

    VPLMutex_Lock(&handle->mutex);
    if(handle->numBlocks <= blockNum){  // Out of range
        goto fail;
    }

    index = blockNum/8;
    offset = blockNum%8;
    mask = 1;
    mask = mask << offset;
    mask_inv = ~mask;

    if((handle->valid_bitmap[index] & (u8)mask) != 0) {  // Bit actually needs clearing
        ssize_t numwrote;
        handle->numValidBlocks--;
        handle->valid_bitmap[index] = handle->valid_bitmap[index] & mask_inv;
        // TODO check return value?
        numwrote = VPLFile_WriteAt(handle->mapfd, &handle->valid_bitmap[index], 1, sizeof(struct contentfile_map_header)+index);
        (void)numwrote;
    }
    VPLMutex_Unlock(&handle->mutex);
    return 0;

fail:
    VPLMutex_Unlock(&handle->mutex);
    return -1;
}

int
contentfile_write(contentfile_handle_t handle, u32 blockNum, u32 numBlocks, const u8* buf)
{
    int fail_rv = CONTENTFILE_ERR;
    u32 numBytesWritten = 0;
    int i;

    if(handle->numBlocks <= blockNum || 
       handle->numBlocks <= blockNum + (numBlocks - 1)){  // Out of range
        goto fail;
    }

    for(i = 0; i < numBlocks; i++) {
        // Bug 6069: This code is dead. The increment below is hidden to remove all references to atomic operations.
        #if 0
        // TODO: expose atomic add?
        VPL_ATOMIC_INC_UNSIGNED32(handle->num_blk_writes);
        #endif
        assert(0);
    }
    numBytesWritten = VPLFile_WriteAt(handle->blkfd,
                                      (void*)buf, // TODO: bug in NDK's unistd.h: http://code.google.com/p/android/issues/detail?id=8908
                                      handle->blocksize * numBlocks,
                                      (u64)((u64)blockNum * (u64)handle->blocksize));
    if(numBytesWritten == -1) {
        LOG_ERROR("Failed to write block data to cache: %s(%d)",
                  strerror(errno), errno);
        goto fail;
    }

#if !USE_O_DIRECT
    if(handle->b_autosync){ VPLFile_Sync(handle->blkfd); }
#endif


    for(i = 0; i < numBlocks; i++) {
        fail_rv = contentfile_set_valid_bit(handle, blockNum + i);

        if(fail_rv != 0){
            LOG_ERROR("contentfile_set_valid_bit: blockNum:%d numBlocks:%d rv:%d",
                      blockNum, handle->numBlocks, fail_rv);
            goto fail;
        }
    }
    return numBytesWritten;
fail:
    return fail_rv;
}

int
contentfile_read(contentfile_handle_t handle, u32 blockNum, u8* buf_out)
{
    int valid;
    int rc;
    u32 numBytesRead = 0;
    rc = contentfile_get_valid_bit(handle, blockNum, &valid);
    if(valid && rc==0){
        numBytesRead = VPLFile_ReadAt(handle->blkfd,
                                      buf_out,
                                      handle->blocksize,
                                      (u64)((u64)handle->blocksize * (u64)blockNum));
        if(numBytesRead == -1) {
            LOG_ERROR("Read error for block %d: %s(%d).",
                      blockNum, strerror(errno), errno);
        }
        else if(numBytesRead != handle->blocksize) {
            LOG_ERROR("Read error for block %d, only %d bytes out %d bytes read.",
                      blockNum, numBytesRead, handle->blocksize);
        }
        else {
            return numBytesRead;
        }
    }
    
    return -1;
}

int
contentfile_invalidate(contentfile_handle_t handle, u32 blockNum)
{
    int rc = contentfile_clear_valid_bit(handle, blockNum);
    return rc;
}

int contentfile_rename(const char* old_name, 
                       const char* new_name)
{
    int rv = 0;
    char* old_file_name = NULL;
    char* new_file_name = NULL;

    // Rename each contentfile component.
    
    // blocks file
    old_file_name = contentfile_alloc_blk_file_name(old_name);
    new_file_name = contentfile_alloc_blk_file_name(new_name);
    if(old_file_name == NULL || new_file_name == NULL) {
        LOG_ERROR("Could not allocate blk names to rename %s to %s.",
                  old_name, new_name);
        rv = CONTENTFILE_CREATE_FILE_ERR;
        goto exit;
    }

    // Don't care if it's not there to be renamed.
    VPLFile_Rename(old_file_name, new_file_name);

    free(old_file_name);
    free(new_file_name);
    old_file_name = NULL;
    new_file_name = NULL;

    // map file
    old_file_name = contentfile_alloc_map_file_name(old_name);
    new_file_name = contentfile_alloc_map_file_name(new_name);
    if(old_file_name == NULL || new_file_name == NULL) {
        LOG_ERROR("Could not allocate blk names to rename %s to %s.",
                  old_name, new_name);
        rv = CONTENTFILE_CREATE_FILE_ERR;
        goto exit;
    }

    // Don't care if it's not there to be renamed.
    VPLFile_Rename(old_file_name, new_file_name);

 exit:
    if(old_file_name) free(old_file_name);
    if(new_file_name) free(new_file_name);

    return rv;
}

static void *aligned_malloc(size_t size, size_t alignment)
{
#ifdef WIN32
#if __MSVCRT_VERSION__ >= 0x0700
    return _aligned_malloc(size, alignment);
#else
    return __mingw_aligned_malloc(size, alignment);
#endif
#elif defined(ANDROID)
    return memalign(alignment, size);
#else
    void *p;
    int rc = posix_memalign(&p, alignment, size);
    return rc == 0 ? p : NULL;
#endif
}

int contentfile_copy(contentfile_handle_t handle,
                     const char* new_name)
{
    int rv = 0;
    char* new_blk_name = NULL;
    char* new_map_name = NULL;
    VPLFile_handle_t blkfd = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t mapfd = VPLFILE_INVALID_HANDLE;
    struct contentfile_map_header header;
    u32 i, written;
    char* buf = NULL;

    // Create new files and write the current contentfile data to them.

    buf = aligned_malloc(handle->blocksize, 4096);
    if(buf == NULL) {
        LOG_ERROR("Could not allocate aligned block buffer to copy contentfile %s.",
                  new_name);
        rv = CONTENTFILE_MEMORY_ERR;
        goto fail_buf;
    }

    // Compose file names.
    new_blk_name = contentfile_alloc_blk_file_name(new_name);
    new_map_name = contentfile_alloc_map_file_name(new_name);
    if(new_blk_name == NULL || new_map_name == NULL) {
        LOG_ERROR("Failed to allocate file names to copy contentfile %s.",
                  new_name);
        rv = CONTENTFILE_MEMORY_ERR;
        goto fail_make_names;
    }

    // Open files (new files).
    blkfd = VPLFile_Open(new_blk_name, 
#if USE_O_DIRECT
                         VPLFILE_OPENFLAG_DIRECT | 
#endif
                         VPLFILE_OPENFLAG_WRITEONLY | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_CREATE, 0600);
    mapfd = VPLFile_Open(new_map_name, VPLFILE_OPENFLAG_WRITEONLY | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_CREATE, 0600);

    if(!VPLFile_IsValidHandle(blkfd) || !VPLFile_IsValidHandle(mapfd)) {
        LOG_ERROR("Failed to open all files to copy contentfile %s.",
                  new_name);
        rv = CONTENTFILE_ERR;
        goto fail_open;
    }

    // Write block data first, then map data.
    for(i = 0; i < handle->numBlocks; i++) {
        int valid;
        
        contentfile_get_valid_bit(handle, i, &valid);
        if(valid) {
            int readIn = VPLFile_ReadAt(handle->blkfd, buf, handle->blocksize,
                                        (u64)((u64)i * (u64)handle->blocksize));
            if(readIn != handle->blocksize) {
                LOG_ERROR("Failed to read-in block %d when copying contentfile %s: %s(%d).",
                          i, new_name, strerror(errno), errno);
                rv = CONTENTFILE_ERR;
                goto fail_read_block;
            }
            written = VPLFile_WriteAt(blkfd, buf, handle->blocksize,
                                      (u64)((u64)i * (u64)handle->blocksize));
            if(written != handle->blocksize) {
                LOG_ERROR("Failed to write block %d when copying contentfile %s: %s(%d).",
                          i, new_name, strerror(errno), errno);
                rv = CONTENTFILE_ERR;
                goto fail_write_block;
            }

        }
    }

#if !USE_O_DIRECT
    VPLFile_Sync(blkfd);
#endif

    // map data header
    header.version   = CONTENTFILE_VERSION;
    header.blocksize = handle->blocksize;
    header.numBlocks = handle->numBlocks;
    written = VPLFile_Write(mapfd, &header, sizeof(header));
    if(written != sizeof(header)) {
        LOG_ERROR("Failed to write map header for contentfile copy %s %s(%d).",
                  new_name, strerror(errno), errno);  
        rv = CONTENTFILE_ERR;
        goto fail_write_header;
    }
    // map data bitmask
    written = VPLFile_Write(mapfd, handle->valid_bitmap, 
                            BLOCKS_TO_BITMAP_BYTES(handle->numBlocks));
    if(written != BLOCKS_TO_BITMAP_BYTES(handle->numBlocks)) {
        LOG_ERROR("Failed to write map bitmask for contentfile copy %s %s(%d).",
                  new_name, strerror(errno), errno);        
        rv = CONTENTFILE_ERR;
        goto fail_write_bitmask;
    }
    VPLFile_Sync(mapfd);

 fail_write_bitmask:
 fail_write_header:
 fail_write_block:
 fail_read_block:    
    VPLFile_Close(blkfd);
    VPLFile_Close(mapfd);
 fail_open:
 fail_make_names:
    if(new_blk_name) free(new_blk_name);
    if(new_map_name) free(new_map_name);
    free(buf);
 fail_buf:
    return rv;
}

int contentfile_delete(const char* name)
{
    int rv = 0;
    char* blk_name = NULL;
    char* map_name = NULL;

    // Delete all contentfile components.

    // Compose file names.
    blk_name = contentfile_alloc_blk_file_name(name);
    map_name = contentfile_alloc_map_file_name(name);
    if(blk_name == NULL || map_name == NULL) {
        LOG_ERROR("Failed to allocate file names to copy contentfile %s.",
                  name);
        rv = CONTENTFILE_MEMORY_ERR;
        goto fail_make_names;
    }

    VPLFile_Delete(map_name);
    VPLFile_Delete(blk_name);

 fail_make_names:
    if(blk_name) free(blk_name);
    if(map_name) free(map_name);

    return rv;
}

void contentfile_get_progress(contentfile_handle_t handle,
                              u32 *blockSize_out,
                              u32 *blocksValid_out,
                              u32 *blocksTotal_out)
{
    *blockSize_out = handle->blocksize;
    *blocksValid_out = handle->numValidBlocks;
    *blocksTotal_out = handle->numBlocks;
}
