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
/// vssts_shim.cpp
///
/// VSSI-TS shim layer on top of either VSSI or TS

#include "vplu_types.h"
#include "vpl_th.h"
#include "vplex_trace.h"
#include "vssts.hpp"
#include "vssts_error.hpp"
#include <cstdlib>

// reload as the real vssits layer
#undef __VSSTS_HPP__
#undef __VSSTS_TYPES_HPP__
#define IS_VSSTS   1
#include "vssts.hpp"
#undef IS_VSSTS

// reload as the vssits-wrapper layer
#undef __VSSTS_HPP__
#undef __VSSTS_TYPES_HPP__
#define IS_VSSTS_WRAPPER   1
#include "vssts.hpp"
#undef IS_VSSTS_WRAPPER

/**
 * Debug param for testing purpose only
 * This mask specify particular routing types
 * need to be disabled in internal VSSI library.
 * NOTE: This not a thread safe param.
 *       Modify this only before calling VSSI_OpenObjectTS.
 *
 * 000: Enable all
 * 001: Disable direct internal routing
 * 010: Disable direct external routing
 * 100: Disable proxy routing
 */
int __debug_disable_route_mask = 0;

typedef struct jump_table_s {
    u64 (*getVersion)(VSSI_Object handle);
    VSSI_Dirent2* (*readDir2)(VSSI_Dir2 dir);
    void (*rewindDir2)(VSSI_Dir2 dir);
    void (*closeDir2)(VSSI_Dir2 dir);
    void (*delete2)(u64 user_id,
                   u64 dataset_id,
                   void* ctx,
                   VSSI_Callback callback);
    void (*openObjectTS)(u64 user_id,
                         u64 dataset_id,
                         u8 mode,
                         VSSI_Object* handle,
                         void* ctx,
                         VSSI_Callback callback);
    void (*closeObject)(VSSI_Object handle,
                        void* ctx,
                        VSSI_Callback callback);
    void (*mkDir2)(VSSI_Object handle,
                   const char* name,
                   u32 attrs,
                   void* ctx,
                   VSSI_Callback callback);
    void (*openDir2)(VSSI_Object handle,
                     const char* name,
                     VSSI_Dir2* dir,
                     void* ctx,
                     VSSI_Callback callback);
    void (*stat2)(VSSI_Object handle,
                  const char* name,
                  VSSI_Dirent2** stats,
                  void* ctx,
                  VSSI_Callback callback);
    void (*chmod)(VSSI_Object handle,
                  const char* name,
                  u32 attrs,
                  u32 attrs_mask,
                  void* ctx,
                  VSSI_Callback callback);
    void (*remove)(VSSI_Object handle,
                   const char* name,
                   void* ctx,
                   VSSI_Callback callback);
    void (*rename2)(VSSI_Object object,
                    const char* name,
                    const char* new_name,
                    u32 flags,
                    void* ctx,
                    VSSI_Callback callback);
    void (*setTimes)(VSSI_Object handle,
                     const char* name,
                     VPLTime_t ctime,
                     VPLTime_t mtime,
                     void* ctx,
                     VSSI_Callback callback);
    void (*getSpace)(VSSI_Object handle,
                     u64* disk_size,
                     u64* dataset_size,
                     u64* avail_size, 
                     void* ctx,
                     VSSI_Callback callback);
    void (*openFile)(VSSI_Object handle,
                     const char* name,
                     u32 flags,
                     u32 attrs,
                     VSSI_File* file,
                     void* ctx,
                     VSSI_Callback callback);
    void (*closeFile)(VSSI_File file,
                      void* ctx,
                      VSSI_Callback callback);
    void (*readFile)(VSSI_File file,
                     u64 offset,
                     u32* length,
                     char* buf,
                     void* ctx,
                     VSSI_Callback callback);
    void (*writeFile)(VSSI_File file,
                      u64 offset,
                      u32* length,
                      const char* buf,
                      void* ctx,
                      VSSI_Callback callback);
    void (*truncateFile)(VSSI_File file,
                         u64 offset,
                         void* ctx,
                         VSSI_Callback callback);
    void (*chmodFile)(VSSI_File file,
                      u32 attrs,
                      u32 attrs_mask,
                      void* ctx,
                      VSSI_Callback callback);
    void (*releaseFile)(VSSI_File file,
                        void* ctx,
                        VSSI_Callback callback);
    VSSI_ServerFileId (*getServerFileId)(VSSI_File file);
    void (*getNotifyEvents)(VSSI_Object handle,
                            VSSI_NotifyMask* mask_out,
                            void* ctx,
                            VSSI_Callback callback);
    void (*setNotifyEvents)(VSSI_Object handle,
                            VSSI_NotifyMask* mask_in_out,
                            void* notify_ctx,
                            VSSI_NotifyCallback notify_callback,
                            void* ctx,
                            VSSI_Callback callback);
    void (*setFileLockState)(VSSI_File file,
                             VSSI_FileLockState lock_state,
                             void* ctx,
                             VSSI_Callback callback);
    void (*getFileLockState)(VSSI_File file,
                             VSSI_FileLockState* lock_state,
                             void* ctx,
                             VSSI_Callback callback);
    void (*setByteRangeLock)(VSSI_File file,
                             VSSI_ByteRangeLock* br_lock,
                             u32 flags,
                             void* ctx,
                             VSSI_Callback callback);
} jump_table_t;

typedef struct vssi_object_s {
    bool                        is_ts;
    VSSI_Object                 handle;
    jump_table_t*               jump;
} vssi_object_t;

typedef struct vssi_dir_s {
    vssi_object_t*              obj;
    VSSI_Dir2                   dir;
} vssi_dir_t;

typedef struct vssi_file_s {
    vssi_object_t*              obj;
    VSSI_File                   file;
} vssi_file_t;

typedef struct shim_open_ctxt_s {
    vssi_object_t*              obj;
    VSSI_Object*                handle;
    void*                       ctx;
    VSSI_Callback               callback;
} shim_open_ctxt_t;

typedef struct shim_ofile_ctxt_s {
    vssi_object_t*              obj;
    VSSI_File                   file;
    VSSI_File*                  ret_file;
    void*                       ctx;
    VSSI_Callback               callback;
} shim_ofile_ctxt_t;

typedef struct shim_cfile_ctxt_s {
    vssi_file_t*                vf;
    void*                       ctx;
    VSSI_Callback               callback;
} shim_cfile_ctxt_t;

typedef struct shim_odir_ctxt_s {
    vssi_object_t*              obj;
    VSSI_Dir2                   dir;
    VSSI_Dir2*                  ret_dir;
    void*                       ctx;
    VSSI_Callback               callback;
} shim_odir_ctxt_t;

using namespace std;

jump_table_t jt_wrapper = {
    vssts_wrapper::VSSI_GetVersion,
    (VSSI_Dirent2*(*)(VSSI_Dir2))vssts_wrapper::VSSI_ReadDir2,
    vssts_wrapper::VSSI_RewindDir2,
    vssts_wrapper::VSSI_CloseDir2,
    vssts_wrapper::VSSI_Delete_Deprecated,
    vssts_wrapper::VSSI_OpenObjectTS,
    vssts_wrapper::VSSI_CloseObject,
    vssts_wrapper::VSSI_MkDir2,
    vssts_wrapper::VSSI_OpenDir2,
    (void(*)(VSSI_Object handle,
                  const char* name,
                  VSSI_Dirent2** stats,
                  void* ctx,
                  VSSI_Callback callback))vssts_wrapper::VSSI_Stat2,
    vssts_wrapper::VSSI_Chmod,
    vssts_wrapper::VSSI_Remove,
    vssts_wrapper::VSSI_Rename2,
    vssts_wrapper::VSSI_SetTimes,
    vssts_wrapper::VSSI_GetSpace,
    vssts_wrapper::VSSI_OpenFile,
    vssts_wrapper::VSSI_CloseFile,
    vssts_wrapper::VSSI_ReadFile,
    vssts_wrapper::VSSI_WriteFile,
    vssts_wrapper::VSSI_TruncateFile,
    vssts_wrapper::VSSI_ChmodFile,
    vssts_wrapper::VSSI_ReleaseFile,
    vssts_wrapper::VSSI_GetServerFileId,
    vssts_wrapper::VSSI_GetNotifyEvents,
    vssts_wrapper::VSSI_SetNotifyEvents,
    vssts_wrapper::VSSI_SetFileLockState,
    vssts_wrapper::VSSI_GetFileLockState,
    (void (*)(VSSI_File file,
              VSSI_ByteRangeLock* br_lock,
              u32 flags,
              void* ctx,
              VSSI_Callback callback))vssts_wrapper::VSSI_SetByteRangeLock
};

jump_table_t jt_ts = {
    vssts::VSSI_GetVersion,
    (VSSI_Dirent2*(*)(VSSI_Dir2))vssts::VSSI_ReadDir2,
    vssts::VSSI_RewindDir2,
    vssts::VSSI_CloseDir2,
    vssts::VSSI_Delete_Deprecated,
    vssts::VSSI_OpenObjectTS,
    vssts::VSSI_CloseObject,
    vssts::VSSI_MkDir2,
    vssts::VSSI_OpenDir2,
    (void(*)(VSSI_Object, const char*, VSSI_Dirent2**, void*, VSSI_Callback))
        vssts::VSSI_Stat2,
    vssts::VSSI_Chmod,
    vssts::VSSI_Remove,
    vssts::VSSI_Rename2,
    vssts::VSSI_SetTimes,
    vssts::VSSI_GetSpace,
    vssts::VSSI_OpenFile,
    vssts::VSSI_CloseFile,
    vssts::VSSI_ReadFile,
    vssts::VSSI_WriteFile,
    vssts::VSSI_TruncateFile,
    vssts::VSSI_ChmodFile,
    vssts::VSSI_ReleaseFile,
    vssts::VSSI_GetServerFileId,
    vssts::VSSI_GetNotifyEvents,
    vssts::VSSI_SetNotifyEvents,
    vssts::VSSI_SetFileLockState,
    vssts::VSSI_GetFileLockState,
    (void (*)(VSSI_File file,
              VSSI_ByteRangeLock* br_lock,
              u32 flags,
              void* ctx,
              VSSI_Callback callback))vssts::VSSI_SetByteRangeLock
};

VSSI_Result VSSI_Init(u64 app_id)
{
    VSSI_Result result;

    // Initialize both libraries.
    result = vssts::VSSI_Init(app_id);
    if ( result != VSSI_SUCCESS ) {
        return result;
    }

    result = vssts_wrapper::VSSI_Init(app_id);
    if ( result != VSSI_SUCCESS ) {
        vssts::VSSI_Cleanup();
        goto done;
    }

    // For the wrapper we need to perform additional work such as start up
    // a thread to handle the polling loop. This will need to be a separate
    // API specific to the wrapper.

done:
    return result;
}

void VSSI_Cleanup(void)
{
    vssts::VSSI_Cleanup();
    vssts_wrapper::VSSI_Cleanup();
}

u64 VSSI_GetVersion(VSSI_Object handle)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    return (obj->jump->getVersion)(obj->handle);
}

VSSI_Dirent2* VSSI_ReadDir2(VSSI_Dir2 dir)
{
    vssi_dir_t* vd = (vssi_dir_t*)dir;
    return (vd->obj->jump->readDir2)(vd->dir);
}

void VSSI_RewindDir2(VSSI_Dir2 dir)
{
    vssi_dir_t* vd = (vssi_dir_t*)dir;
    return (vd->obj->jump->rewindDir2)(vd->dir);
}

void VSSI_CloseDir2(VSSI_Dir2 dir)
{
    vssi_dir_t* vd = (vssi_dir_t*)dir;
    (vd->obj->jump->closeDir2)(vd->dir);
    delete vd;
}

static void shim_open_callback(void* ctxt, VSSI_Result result)
{
    shim_open_ctxt_t* open_ctxt = (shim_open_ctxt_t*)ctxt;

    // Copy back to caller's handle only on success
    if ( result == VSSI_SUCCESS ) {
        *(open_ctxt->handle) = (VSSI_Object)open_ctxt->obj;
    }
    else {
        delete open_ctxt->obj;
    }
    (open_ctxt->callback)(open_ctxt->ctx, result);
    delete open_ctxt;
}

void VSSI_Delete_Deprecated(u64 user_id,
                  u64 dataset_id,
                  void* ctx,
                  VSSI_Callback callback)
{
    bool is_ts = vssts_wrapper::VSSI_DatasetIsNewVssi(user_id, dataset_id);
    jump_table_t* jump = is_ts ? &jt_ts : &jt_wrapper;

    (jump->delete2)(user_id, dataset_id, ctx, callback);
}

void VSSI_OpenObjectTS(u64 user_id,
                       u64 dataset_id,
                       u8 mode,
                       VSSI_Object* handle,
                       void* ctx,
                       VSSI_Callback callback)
{
    shim_open_ctxt_t* open_ctxt = new shim_open_ctxt_t;
    vssi_object_t* obj = new vssi_object_t;

    open_ctxt->obj = obj;
    open_ctxt->handle = handle;
    open_ctxt->ctx = ctx;
    open_ctxt->callback = callback;

    // Look up the device associated with this dataset.
    obj->is_ts = vssts_wrapper::VSSI_DatasetIsNewVssi(user_id, dataset_id);
    obj->jump = ( obj->is_ts ) ? &jt_ts : &jt_wrapper;

    (obj->jump->openObjectTS)(user_id,
                              dataset_id,
                              mode,
                              &obj->handle,
                              open_ctxt,
                              shim_open_callback);

    // Note: The handle is set prior to calling the callback. This is kind
    // of weird, but there you go...
}

void VSSI_CloseObject(VSSI_Object handle,
                      void* ctx,
                      VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    (obj->jump->closeObject)(obj->handle, ctx, callback);
    delete obj;
}

void VSSI_MkDir2(VSSI_Object handle,
                 const char* name,
                 u32 attrs,
                 void* ctx,
                 VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    (obj->jump->mkDir2)(obj->handle, name, attrs, ctx, callback);
}

static void shim_odir_callback(void* ctxt, VSSI_Result result)
{
    shim_odir_ctxt_t* dir_ctxt = (shim_odir_ctxt_t*)ctxt;

    // This return of an error but still success really broke
    // the mold, sigh.
    if ( result == VSSI_SUCCESS ) {
        vssi_dir_t* dir = new vssi_dir_t;

        dir->obj = dir_ctxt->obj;
        dir->dir = dir_ctxt->dir;
        *(dir_ctxt->ret_dir) = (VSSI_Dir2)dir;
    }
    else {
        *(dir_ctxt->ret_dir) = NULL;
    }
    (dir_ctxt->callback)(dir_ctxt->ctx, result);
    delete dir_ctxt;
}

void VSSI_OpenDir2(VSSI_Object handle,
                   const char* name,
                   VSSI_Dir2* dir,
                   void* ctx,
                   VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    shim_odir_ctxt_t* ctxt = new shim_odir_ctxt_t;

    ctxt->obj = obj;
    ctxt->ret_dir = dir;
    ctxt->ctx = ctx;
    ctxt->callback = callback;

    (obj->jump->openDir2)(obj->handle, name, &ctxt->dir, ctxt,
                          shim_odir_callback);
}

void VSSI_Stat2(VSSI_Object handle,
                const char* name,
                VSSI_Dirent2** stats,
                void* ctx,
                VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    (obj->jump->stat2)(obj->handle, name, stats, ctx, callback);
}

void VSSI_Chmod(VSSI_Object object,
                const char* name,
                u32 attrs,
                u32 attrs_mask,
                void* ctx,
                VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)object;
    (obj->jump->chmod)(obj->handle, name, attrs, attrs_mask, ctx, callback);
}

void VSSI_Remove(VSSI_Object handle,
                 const char* name,
                 void* ctx,
                 VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    (obj->jump->remove)(obj->handle, name, ctx, callback);
}

void VSSI_Rename2(VSSI_Object object,
                  const char* name,
                  const char* new_name,
                  u32 flags,
                  void* ctx,
                  VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)object;
    (obj->jump->rename2)(obj->handle, name, new_name, flags, ctx, callback);
}

void VSSI_SetTimes(VSSI_Object handle,
                   const char* name,
                   VPLTime_t ctime,
                   VPLTime_t mtime,
                   void* ctx,
                   VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    (obj->jump->setTimes)(obj->handle, name, ctime, mtime, ctx, callback);
}

void VSSI_GetSpace(VSSI_Object handle,
                   u64* disk_size,
                   u64* dataset_size,
                   u64* avail_size, 
                   void* ctx,
                   VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    (obj->jump->getSpace)(obj->handle,
                          disk_size,
                          dataset_size,
                          avail_size,
                          ctx,
                          callback);
}

/// File Handle APIs
static void shim_ofile_callback(void* ctxt, VSSI_Result result)
{
    shim_ofile_ctxt_t* file_ctxt = (shim_ofile_ctxt_t*)ctxt;

    // This return of an error but still success really broke
    // the mold, sigh.
    if ( (result == VSSI_SUCCESS) || (result == VSSI_EXISTS) ) {
        vssi_file_t* file = new vssi_file_t;

        file->obj = file_ctxt->obj;
        file->file = file_ctxt->file;
        *(file_ctxt->ret_file) = (VSSI_File)file;
    }
    else {
        *(file_ctxt->ret_file) = NULL;
    }
    (file_ctxt->callback)(file_ctxt->ctx, result);
    delete file_ctxt;
}

void VSSI_OpenFile(VSSI_Object handle,
                   const char* name,
                   u32 flags,
                   u32 attrs,
                   VSSI_File* file,
                   void* ctx,
                   VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    shim_ofile_ctxt_t* ctxt = new shim_ofile_ctxt_t;

    ctxt->obj = obj;
    ctxt->ret_file = file;
    ctxt->ctx = ctx;
    ctxt->callback = callback;

    (obj->jump->openFile)(obj->handle, name, flags, attrs, &ctxt->file, ctxt, 
                          shim_ofile_callback);
}

static void shim_cfile_callback(void* ctxt, VSSI_Result result)
{
    shim_cfile_ctxt_t* file_ctxt = (shim_cfile_ctxt_t*)ctxt;

    // callback will only be called when the result is actually returned from server
    // we should clean up file on every rv except VSSI_COMM, for which we allow a retry
    if (result != VSSI_COMM) {
        vssi_file_t* file = (vssi_file_t*)file_ctxt->vf;
        file_ctxt->vf = NULL;
        delete file;
    }

    (file_ctxt->callback)(file_ctxt->ctx, result);
    delete file_ctxt;
}

void VSSI_CloseFile(VSSI_File file,
                    void* ctx,
                    VSSI_Callback callback)
{
    vssi_file_t* vf = (vssi_file_t*)file;
    shim_cfile_ctxt_t* ctxt = new shim_cfile_ctxt_t;

    ctxt->callback = callback;
    ctxt->ctx = ctx;
    ctxt->vf = vf;

    (vf->obj->jump->closeFile)(vf->file, ctxt, shim_cfile_callback);
}

void VSSI_ReadFile(VSSI_File file,
                   u64 offset,
                   u32* length,
                   char* buf,
                   void* ctx,
                   VSSI_Callback callback)
{
    vssi_file_t* vf = (vssi_file_t*)file;
    (vf->obj->jump->readFile)(vf->file, offset, length, buf, ctx, callback);
}

void VSSI_WriteFile(VSSI_File file,
                    u64 offset,
                    u32* length,
                    const char* buf,
                    void* ctx,
                    VSSI_Callback callback)
{
    vssi_file_t* vf = (vssi_file_t*)file;
    (vf->obj->jump->writeFile)(vf->file, offset, length, buf, ctx, callback);
}

void VSSI_TruncateFile(VSSI_File file,
                       u64 offset,
                       void* ctx,
                       VSSI_Callback callback)
{
    vssi_file_t* vf = (vssi_file_t*)file;
    (vf->obj->jump->truncateFile)(vf->file, offset, ctx, callback);
}

void VSSI_ChmodFile(VSSI_File file,
                    u32 attrs,
                    u32 attrs_mask,
                    void* ctx,
                    VSSI_Callback callback)
{
    vssi_file_t* vf = (vssi_file_t*)file;
    (vf->obj->jump->chmodFile)(vf->file, attrs, attrs_mask, ctx, callback);
}

void VSSI_ReleaseFile(VSSI_File file,
                      void* ctx,
                      VSSI_Callback callback)
{
    vssi_file_t* vf = (vssi_file_t*)file;
    (vf->obj->jump->releaseFile)(vf->file, ctx, callback);
}

VSSI_ServerFileId VSSI_GetServerFileId(VSSI_File file)
{
    vssi_file_t* vf = (vssi_file_t*)file;
    return (vf->obj->jump->getServerFileId)(vf->file);
}

void VSSI_GetNotifyEvents(VSSI_Object handle,
                          VSSI_NotifyMask* mask_out,
                          void* ctx,
                          VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    (obj->jump->getNotifyEvents)(obj->handle, mask_out, ctx, callback);
}

void VSSI_SetNotifyEvents(VSSI_Object handle,
                          VSSI_NotifyMask* mask_in_out,
                          void* notify_ctx,
                          VSSI_NotifyCallback notify_callback,
                          void* ctx,
                          VSSI_Callback callback)
{
    vssi_object_t* obj = (vssi_object_t*)handle;
    (obj->jump->setNotifyEvents)(obj->handle,
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
    vssi_file_t* vf = (vssi_file_t*)file;
    (vf->obj->jump->setFileLockState)(vf->file, lock_state, ctx, callback);
}

void VSSI_GetFileLockState(VSSI_File file,
                           VSSI_FileLockState* lock_state,
                           void* ctx,
                           VSSI_Callback callback)
{
    vssi_file_t* vf = (vssi_file_t*)file;
    (vf->obj->jump->getFileLockState)(vf->file, lock_state, ctx, callback);
}

void VSSI_SetByteRangeLock(VSSI_File file,
                           VSSI_ByteRangeLock* br_lock,
                           u32 flags,
                           void* ctx,
                           VSSI_Callback callback)
{
    vssi_file_t* vf = (vssi_file_t*)file;
    (vf->obj->jump->setByteRangeLock)(vf->file, br_lock, flags, ctx, callback);
}

void VSSI_NetworkDown(void)
{
    // vssi networkdown
    vssts_wrapper::VSSI_NetworkDown();

    // ts networkdown
    vssts::VSSI_NetworkDown();
}
