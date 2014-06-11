/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

///
/// vssts_wrapper.cpp
///
/// VSSI-TS wrapper layer on top of original VSSI

#include "vplu_types.h"
#include "vpl_th.h"
#include "vplex_trace.h"
#include "vssi.h"
#include <cstdlib>

#define IS_VSSTS_WRAPPER  1
#include "vssts.hpp"
#undef IS_VSSTS_WRAPPER

#include "vssts_error.hpp"
#include "wrapper_glue.hpp"

using namespace std;

namespace vssts_wrapper {

VSSI_Result VSSI_Init(u64 app_id)
{
    return do_vssi_setup(app_id);
}

void VSSI_Cleanup(void)
{
    do_vssi_cleanup();
}

u64 VSSI_GetVersion(VSSI_Object handle)
{
    return ::VSSI_GetVersion(handle);
}

VSSI_Dirent2* VSSI_ReadDir2(VSSI_Dir2 dir)
{
    return (VSSI_Dirent2*)::VSSI_ReadDir2(dir);
}

void VSSI_RewindDir2(VSSI_Dir2 dir)
{
    ::VSSI_RewindDir2(dir);
}

void VSSI_CloseDir2(VSSI_Dir2 dir)
{
    ::VSSI_CloseDir2(dir);
}

void VSSI_Delete_Deprecated(u64 user_id,
                  u64 dataset_id,
                  void* ctx,
                  VSSI_Callback callback)
{
    do_vssi_delete2(user_id, dataset_id, ctx, callback);
}

void VSSI_OpenObjectTS(u64 user_id,
                       u64 dataset_id,
                       u8 mode,
                       VSSI_Object* handle,
                       void* ctx,
                       VSSI_Callback callback)
{
    do_vssi_open_object2(user_id, dataset_id, mode, handle, ctx, callback);
}

void VSSI_CloseObject(VSSI_Object handle,
                      void* ctx,
                      VSSI_Callback callback)
{
    ::VSSI_CloseObject(handle, ctx, callback);
}

void VSSI_MkDir2(VSSI_Object handle,
                 const char* name,
                 u32 attrs,
                 void* ctx,
                 VSSI_Callback callback)
{
    ::VSSI_MkDir2(handle, name, attrs, ctx, callback);
}

void VSSI_OpenDir2(VSSI_Object handle,
                   const char* name,
                   VSSI_Dir2* dir,
                   void* ctx,
                   VSSI_Callback callback)
{
    ::VSSI_OpenDir2(handle, name, dir, ctx, callback);
}

void VSSI_Stat2(VSSI_Object handle,
                const char* name,
                VSSI_Dirent2** stats,
                void* ctx,
                VSSI_Callback callback)
{
    ::VSSI_Stat2(handle, name, (::VSSI_Dirent2**)stats, ctx, callback);
}

void VSSI_Chmod(VSSI_Object object,
                const char* name,
                u32 attrs,
                u32 attrs_mask,
                void* ctx,
                VSSI_Callback callback)
{
    ::VSSI_Chmod(object, name, attrs, attrs_mask, ctx, callback);
}

void VSSI_Remove(VSSI_Object handle,
                 const char* name,
                 void* ctx,
                 VSSI_Callback callback)
{
    ::VSSI_Remove(handle, name, ctx, callback);
}

void VSSI_Rename2(VSSI_Object object,
                  const char* name,
                  const char* new_name,
                  u32 flags,
                  void* ctx,
                  VSSI_Callback callback)
{
    ::VSSI_Rename2(object, name, new_name, flags, ctx, callback);
}

void VSSI_SetTimes(VSSI_Object handle,
                   const char* name,
                   VPLTime_t ctime,
                   VPLTime_t mtime,
                   void* ctx,
                   VSSI_Callback callback)
{
    ::VSSI_SetTimes(handle, name, ctime, mtime, ctx, callback);
}

void VSSI_GetSpace(VSSI_Object handle,
                   u64* disk_size,
                   u64* dataset_size,
                   u64* avail_size, 
                   void* ctx,
                   VSSI_Callback callback)
{
    ::VSSI_GetSpace(handle, disk_size, dataset_size, avail_size, ctx, callback);
}

/// File Handle APIs

void VSSI_OpenFile(VSSI_Object handle,
                   const char* name,
                   u32 flags,
                   u32 attrs,
                   VSSI_File* file,
                   void* ctx,
                   VSSI_Callback callback)
{
    ::VSSI_OpenFile(handle, name, flags, attrs, file, ctx, callback);
}

void VSSI_CloseFile(VSSI_File file,
                    void* ctx,
                    VSSI_Callback callback)
{
    ::VSSI_CloseFile(file, ctx, callback);
}

void VSSI_ReadFile(VSSI_File file,
                   u64 offset,
                   u32* length,
                   char* buf,
                   void* ctx,
                   VSSI_Callback callback)
{
    ::VSSI_ReadFile(file, offset, length, buf, ctx, callback);
}

void VSSI_WriteFile(VSSI_File file,
                    u64 offset,
                    u32* length,
                    const char* buf,
                    void* ctx,
                    VSSI_Callback callback)
{
    ::VSSI_WriteFile(file, offset, length, buf, ctx, callback);
}

void VSSI_TruncateFile(VSSI_File file,
                       u64 offset,
                       void* ctx,
                       VSSI_Callback callback)
{
    ::VSSI_TruncateFile(file, offset, ctx, callback);
}

void VSSI_ChmodFile(VSSI_File file,
                    u32 attrs,
                    u32 attrs_mask,
                    void* ctx,
                    VSSI_Callback callback)
{
    ::VSSI_ChmodFile(file, attrs, attrs_mask, ctx, callback);
}

void VSSI_ReleaseFile(VSSI_File file,
                      void* ctx,
                      VSSI_Callback callback)
{
    ::VSSI_ReleaseFile(file, ctx, callback);
}

VSSI_ServerFileId VSSI_GetServerFileId(VSSI_File file)
{
    return ::VSSI_GetServerFileId(file);
}

void VSSI_GetNotifyEvents(VSSI_Object handle,
                          VSSI_NotifyMask* mask_out,
                          void* ctx,
                          VSSI_Callback callback)
{
    ::VSSI_GetNotifyEvents(handle, mask_out, ctx, callback);
}

void VSSI_SetNotifyEvents(VSSI_Object handle,
                          VSSI_NotifyMask* mask_in_out,
                          void* notify_ctx,
                          VSSI_NotifyCallback notify_callback,
                          void* ctx,
                          VSSI_Callback callback)
{
    ::VSSI_SetNotifyEvents(handle,
                           mask_in_out,
                           notify_ctx,
                           notify_callback,
                           ctx,
                           callback);
}
                                      
void VSSI_SetFileLockState(VSSI_File file,
                           VSSI_FileLockState lock_state,
                           void* ctx,
                           VSSI_Callback callback)
{
    ::VSSI_SetFileLockState(file, lock_state, ctx, callback);
}

void VSSI_GetFileLockState(VSSI_File file,
                           VSSI_FileLockState* lock_state,
                           void* ctx,
                           VSSI_Callback callback)
{
    ::VSSI_GetFileLockState(file, lock_state, ctx, callback);

}

void VSSI_SetByteRangeLock(VSSI_File file,
                           VSSI_ByteRangeLock* br_lock,
                           u32 flags,
                           void* ctx,
                           VSSI_Callback callback)
{
    ::VSSI_SetByteRangeLock(file,
                            (::VSSI_ByteRangeLock*)br_lock,
                            flags,
                            ctx,
                            callback);
}

void VSSI_NetworkDown(void)
{
    ::VSSI_NetworkDown();
}

bool VSSI_DatasetIsNewVssi(u64 user_id, u64 dataset_id)
{
    return dataset_is_new_vssi(user_id, dataset_id);
}

} // namespace
