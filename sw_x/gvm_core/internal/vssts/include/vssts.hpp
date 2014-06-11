/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

///
/// vssts.h
///
/// Virtual Storage Server on Tunnel Services Interface 

#ifndef __VSSTS_HPP__
#define __VSSTS_HPP__

#include "vssts_types.hpp"
#include "vpl_socket.h"
#include "vpl_time.h"

#ifdef IS_VSSTS
namespace vssts {
#endif // IS_VSSTS
#ifdef IS_VSSTS_WRAPPER
namespace vssts_wrapper {
#endif // IS_VSSTS_WRAPPER
//
// Library operations
//
// These operations interact with library structures rather than objects or
// object-components. The operations are synchronous.
//

/// Initialize VSSI internal state variables.
/// @param app_id This is used to modify the device ID so it doesn't conflict with CCD
///               connections via VSSI to the same server. Otherwise the server gets
///               confused about routing responses
///               NOTE: This SHOULD BE REMOVED once the old vssi is totally gone.
/// @return VSSI_SUCCESS on successful init or already initialized.
///         VSSI error code on failure.
/// @note VSSI initializes internal state. Subsequent calls have no effect unless
/// #VSSI_Cleanup() was called to clean up state.
/// Failing to call #VSSI_Init before calling any other VSSI function results
/// in VSSI_INIT error.
VSSI_Result VSSI_Init(u64 app_id);

/// Clean up VSSI internal state for system shutdown.
/// @note All internal state is cleared. Close all objects and end all sessions
/// before calling.
void VSSI_Cleanup(void);

//
// Utility operations
//
// These operations do not require network communication.
//

/// Get the current version of an object.
/// @param handle Object handle
/// @return Current version of the object. 0 on failure to recognize object
///         by handle or if the object has just been created (no version on
///         server).
u64 VSSI_GetVersion(VSSI_Object handle);

/// Read the next entry from an opened directory for an object, returning a
/// pointer to the entry data.
/// @param dir Directory handle, previously opened via
///        #VSSI_OpenDir2().
/// @return Pointer to next directory entry. If no further entries, or dir is
///         not a valid open directory, returns NULL.
VSSI_Dirent2* VSSI_ReadDir2(VSSI_Dir2 dir);

/// Reset the position for reading directory entries to the start of the
/// directory.
/// @param dir Directory handle, previously opened via
///        #VSSI_OpenDir2().
void VSSI_RewindDir2(VSSI_Dir2 dir);

/// Close a directory for an object, freeing local resources.
/// @param dir Directory handle, previously opened via
///        #VSSI_OpenDir2().
void VSSI_CloseDir2(VSSI_Dir2 dir);

/// Delete an object. (Only to be used for testing)
/// @param user_id Owning user's ID
/// @param dataset_id ID of the dataset.
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call.
/// @return Callback is called with the operation's result.
/// @note Deleting an object while it is open will still delete all data.
///        Only objects that could be written by a user can be deleted.
///        Once an object is deleted, it is lost for good on the server.
///        A new object with the same description may be created after
///        deletion, but it will be version 1 on first commit.
void VSSI_Delete_Deprecated(u64 user_id,
                  u64 dataset_id,
                  void* ctx,
                  VSSI_Callback callback);

//
// Session operations
//
// These operations make network connections, possibly reusing existing
// connections when possible. A valid user session is required.
//

/// Open an object with directed route information.
/// @param user_id Owning user's ID
/// @param dataset_id ID of the dataset.
/// @param mode Object access mode. This is one of #VSSI_READONLY or
///              #VSSI_READWRITE. Writable files are always written with
///               overwrite mode, and created on open if not already existing.
/// @param handle Return pointer for object handle on success.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return Callback is called with the operation's result.
void VSSI_OpenObjectTS(u64 user_id,
                       u64 dataset_id,
                       u8 mode,
                       VSSI_Object* handle,
                       void* ctx,
                       VSSI_Callback callback);

//
// Object operations
//
// These operations use existing network connections, reopening them if
// necessary. A valid VSSI_Object is required.
//

/// Close an object.
/// @param handle Handle to the object being closed.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return Callback is called with the operation's result.
void VSSI_CloseObject(VSSI_Object handle,
                      void* ctx,
                      VSSI_Callback callback);

/// Make a new directory component for this object.
/// @param object Handle to the object being changed.
/// @param name Directory to create.
/// @param attrs object attributes associated with the directory
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @note All parent directories needed to make the named directory will be
///       created (like mkdir -p).
/// @note This command is only needed when creating empty directories.
/// @note Special directory names ".." and "." will be interpreted literally,
///       leading to failure.
/// @note This command will destroy existing files if they are in the way of
///       creating the desired directory.
void VSSI_MkDir2(VSSI_Object handle,
                 const char* name,
                 u32 attrs,
                 void* ctx,
                 VSSI_Callback callback);

/// Read the contents of a directory, including contained file/directory
/// attributes and metadata.
/// @param object Handle to the object being accessed.
/// @param name Directory to open.
/// @param dir Pointer to #VSSI_Dir handle for subsequent reading from the open
///        directory
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @note Special directory names ".." and "." will be interpreted literally,
///        leading to failure.
/// @note The current directory information will be read from the server with
///        this command. On success, dir may be used to read the directory
///        entries.
void VSSI_OpenDir2(VSSI_Object handle,
                   const char* name,
                   VSSI_Dir2* dir,
                   void* ctx,
                   VSSI_Callback callback);

/// Read the attributes and metadata for a single file or directory.
/// @param object Handle to the object being accessed.
/// @param name File or directory to stat.
/// @param stats Pointer to #VSSI_Dirent pointer which will return the data.
///        The caller is responsible for freeing this data with free().
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @notes Special directory names ".." and "." will be interpreted literally,
///        leading to failure.
///        The file attributes and metadata will be read from the server with
///        this command. On success, *stats will point to an allocated
///        #VSSI_Dirent. For file metadata, use #VSSI_ReadMetadata() or
///        #VSSI_GetMetadataByType() to read the metadata.
void VSSI_Stat2(VSSI_Object handle,
                const char* name,
                VSSI_Dirent2** stats,
                void* ctx,
                VSSI_Callback callback);

/// Change the attributes for a single file or directory.
/// @param object Handle to the object being accessed.
/// @param name File or directory to stat.
/// @param attrs Attributes to set or clear.
/// @param attrs_mask controls which bits are set or cleared.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
void VSSI_Chmod(VSSI_Object handle,
                const char* name,
                u32 attrs,
                u32 attrs_mask,
                void* ctx,
                VSSI_Callback callback);

/// Delete a file or directory component.
/// @param object Handle to the object being changed.
/// @param name File or directory to delete.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @note This operation functions like "rm -rf".
void VSSI_Remove(VSSI_Object handle,
                 const char* name,
                 void* ctx,
                 VSSI_Callback callback);

/// Rename/move a file or directory component (extended).
/// @param handle Handle to the object being changed.
/// @param flags Option flags to control the behavior of rename.
/// @param name File or directory to rename.
/// @param new_name New name for file or directory.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return #VSSI_INVALID if @a new_name contains a path prefix that matches
///         @a name, or vice-versa.
/// @note This operation functions like "rename(@a name, @a new_name)".
///       If @a name does not exist, this operation has no effect.
void VSSI_Rename2(VSSI_Object object,
                  const char* name,
                  const char* new_name,
                  u32 flags,
                  void* ctx,
                  VSSI_Callback callback);

/// Set the timestamps of an object component.
/// @param object Handle to the object being written
/// @param name File to truncate.
/// @param ctime New creation time (microseconds since 00:00:00.000 1/1/1970)
///        for a component. If 0, @a ctime is ignored.
/// @param mtime New modification time (microseconds since 00:00:00.000 1/1/1970)
///        for a component. If 0, @a mtime is ignored.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return Callback is called with the operation's result.
/// @note This command will create the named file if it doesn't exist, even
///       destroying existing directories to do so.
void VSSI_SetTimes(VSSI_Object handle,
                   const char* name,
                   VPLTime_t ctime,
                   VPLTime_t mtime,
                   void* ctx,
                   VSSI_Callback callback);

/// Get space information.
/// @param handle Handle to the object being changed 
/// @param disk_size Size of entire disk on which dataset is stored.
/// @param dataset_size Size of dataset on disk.
/// @param avail_size Size of space available for this dataset.
/// @param ctx Callback context on operation complete. 
/// @param callback The callback function to call. 
void VSSI_GetSpace(VSSI_Object handle,
                   u64* disk_size,
                   u64* dataset_size,
                   u64* avail_size, 
                   void* ctx,
                   VSSI_Callback callback);

/// File Handle APIs

void VSSI_OpenFile(VSSI_Object handle,
                   const char* name,
                   u32 flags,
                   u32 attrs,
                   VSSI_File* file,
                   void* ctx,
                   VSSI_Callback callback);

void VSSI_CloseFile(VSSI_File file,
                    void* ctx,
                    VSSI_Callback callback);

void VSSI_ReadFile(VSSI_File file,
                   u64 offset,
                   u32* length,
                   char* buf,
                   void* ctx,
                   VSSI_Callback callback);

void VSSI_WriteFile(VSSI_File file,
                    u64 offset,
                    u32* length,
                    const char* buf,
                    void* ctx,
                    VSSI_Callback callback);

void VSSI_TruncateFile(VSSI_File file,
                       u64 offset,
                       void* ctx,
                       VSSI_Callback callback);

void VSSI_ChmodFile(VSSI_File file,
                    u32 attrs,
                    u32 attrs_mask,
                    void* ctx,
                    VSSI_Callback callback);

void VSSI_ReleaseFile(VSSI_File file,
                      void* ctx,
                      VSSI_Callback callback);

VSSI_ServerFileId VSSI_GetServerFileId(VSSI_File file);

/// Get the current async notifications setting for an object.
/// @param object Handle to the object being checked
/// @param mask_out On success, location to write the current notification events mask for the object.
/// @param ctx Callback context on operation complete. 
/// @param callback The callback function to call. 
/// @note This command will retrieve the current notification events active for this object as known by the server.
///       If server connection was lost, there will be no notifications.
///       Use #VSSI_SetNotifyEvents() to set desired notifications and callback to be called when
///       notifications are received.
void VSSI_GetNotifyEvents(VSSI_Object handle,
                          VSSI_NotifyMask* mask_out,
                          void* ctx,
                          VSSI_Callback callback);

/// Set the desired async notifications setting for an object.
/// @param object Handle to the object for which notifications are being changed.
/// @param mask_in_out Pointer to desired notification mask. On success, this mask will be changed to reflect
///       the notifications the server will actually send.
/// @param notify_ctx Callback context for any notifications received from this point forward.
///        This will replace any previous notification context.
/// @param callback Callback function to call when a notification is received from the server.
///        This will replace any previous notification callback.
/// @param ctx Callback context on operation complete. 
/// @param callback The callback function to call. 
/// @note This command will set notifications requested at the server subject to server capabilities.
///       The server connection will not timeout as long as the notification mask has any events set.
///       The server connection will be allowed to timeout if all notification events are unset.
///       Notification type VSSI_NOTIFY_DISCONNECTED_EVENT is implicitly set if any other notification is set.
///       If server connection is lost, the notification callback will be called. This API should
///       be called again to force a re-connection and to make sure the server will send notifications.
void VSSI_SetNotifyEvents(VSSI_Object handle,
                          VSSI_NotifyMask* mask_in_out,
                          void* notify_ctx,
                          VSSI_NotifyCallback notify_callback,
                          void* ctx,
                          VSSI_Callback callback);
                                      
///
/// VSSI File Locking APIs.
///
/// These APIs apply to individual files within a dataset.  Three types of
/// locks are supported:
///   . Access mode locks on an entire file
///   . Access mode locks on byte ranges within a file
///   . Cache state management locks (oplocks)
///

void VSSI_SetFileLockState(VSSI_File file,
                           VSSI_FileLockState lock_state,
                           void* ctx,
                           VSSI_Callback callback);

void VSSI_GetFileLockState(VSSI_File file,
                           VSSI_FileLockState* lock_state,
                           void* ctx,
                           VSSI_Callback callback);

void VSSI_SetByteRangeLock(VSSI_File file,
                           VSSI_ByteRangeLock* br_lock,
                           u32 flags,
                           void* ctx,
                           VSSI_Callback callback);

/// Notify VSSI the network is down.
/// All currently open sockets will be closed and all sent requests will be
/// terminated by calling their callbacks with VSSI_COMM error.
/// After this function is called, it is fine to continue to use the same object and file handles.
void VSSI_NetworkDown(void);

#ifdef IS_VSSTS_WRAPPER
bool VSSI_DatasetIsNewVssi(u64 user_id, u64 datasetid);

}
#endif // IS_VSSTS_WRAPPER
#ifdef IS_VSSTS
}
#endif // IS_VSSTS
#endif // include guard
