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
/// vssi.h
///
/// Virtual Storage Server Interface

#ifndef __VSSI_H__
#define __VSSI_H__

#include "vssi_types.h"
#include "vpl_socket.h"
#include "vpl_time.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Library operations
//
// These operations interact with library structures rather than objects or
// object-components. The operations are synchronous.
//

/// Initialize VSSI internal state variables.
/// @param device_id - This device's device ID.
/// @param notify_change_fn - function to call when VSSI changes the roster
///                           of active sockets.
/// @return VSSI_SUCCESS on successful init or already initialized.
///         VSSI error code on failure.
/// @note VSSI initializes internal state. Subsequent calls have no effect unless
/// #VSSI_Cleanup() was called to clean up state.
/// Failing to call #VSSI_Init before calling any other VSSI function results
/// in VSSI_INIT error.
VSSI_Result VSSI_Init(u64 device_id,
                      void (*notify_change_fn)(void));

/// Clean up VSSI internal state for system shutdown.
/// @note All internal state is cleared. Close all objects and end all sessions
/// before calling.
void VSSI_Cleanup(void);

/// Set a VSSI operating parameter.
/// Parameters may be set at any time, though the change will only affect new
/// operations controlled by the parameter.
/// @param id Parameter ID. See #VSSI_Param.
/// @param value New parameter value. Must be appropriate for the parameter.
/// @return VSSI_SUCCESS if the parmeter is changed.
/// @return VSSI_INVALID if the parameter ID is not valid or value not appropriate.
int VSSI_SetParameter(VSSI_Param id, int value);

/// Get the current value of a VSSI operating parameter.
/// @param id Parameter ID. See #VSSI_Param.
/// @param value_out Pointer to location to write the current parameter value.
/// @return VSSI_SUCCESS if the parmeter value found.
/// @return VSSI_INVALID if the parameter ID is not valid.
int VSSI_GetParameter(VSSI_Param id, int* value_out);

/// For each active socket, call @a fn with the socket.
/// @param fn function taking a VPLSocket_t, flag for recv active, 
///        flag for send active, and context provided to call.
/// @param ctx Pointer to some context for the callback.
/// @note Caller may use this to collect the set of active sockets from VSSI.
void VSSI_ForEachSocket(void (*fn)(VPLSocket_t, int, int, void*),
                        void* ctx);

/// For each active socket, check to see if the socket is ready
/// for receive or send, and if so, handle any activity on the socket.
/// @param recv_ready function taking a VPLSocket_t and provided context that
///        will return true when the socket is ready for receive or disconnect.
/// @param send_ready function taking a VPLSocket_t and provided context that
///        will return true when the socket is ready for send.
/// @param ctx Pointer to some context for the callback.
void VSSI_HandleSockets(int (*recv_ready)(VPLSocket_t, void*),
                        int (*send_ready)(VPLSocket_t, void*),
                        void* ctx);

/// Get the maximum amount of time to wait for a socket to be ready before
/// handling sockets. If this much time passes without socket ready, call
/// #VSSI_HandleSockets() anyway to handle any timeout events.
/// @return Amount of time to wait for socket ready.
///         VPL_TIMEOUT_NONE if no timeout required.
VPLTime_t VSSI_HandleSocketsTimeout(void);

/// Notify VSSI the network is down.
/// All currently open sockets will be closed and all sent requests will be
/// terminated by calling their callbacks with VSSI_COMM error.
/// After this function is called, it is fine to continue to use the same object and file handles.
void VSSI_NetworkDown(void);

/// Registers a session for future use.
/// @param sessionHandle 64-bit login session handle.
/// @param serviceTicket 20-byte service ticket for Virtual Storage services.
/// @return A session handle referring to a VSSI internal session state
///         suitable for use with #VSSI_OpenObject and #VSSI_Delete calls.
///         NULL if a new session could not be created.
VSSI_Session VSSI_RegisterSession(u64 sessionHandle,
                                  const char* serviceTicket);

/// End a previously registered session
/// @param session Handle to the session to end.
/// @return
///  #VSSI_SUCCESS  if session ended successfully
///  #VSSI_INIT     if VSSI not yet initialized. Call #VSSI_Init first.
///  #VSSI_OPENED   if there are outstanding open objects.
///                 Close open objects first.
///  #VSSI_NOTFOUND if the session is not a valid open session.
/// @note Once a session is ended, any objects opened with the session become
/// invalid. Close them first.
VSSI_Result VSSI_EndSession(VSSI_Session session);

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

/// Get the preferred request size of an object as known on open.
/// @param handle Object handle
/// @return Size of preferred read requests for the object.
///         If 0, any size read request may be used.
/// @note Whenever possible, issue read requests for all components of this
/// object with at least the specified request size. Smaller requests may be
/// made, but may incur performance penalties.
u32 VSSI_GetObjectOptimalAccessSize(VSSI_Object object);

/// Read the next entry from an opened directory for an object, returning a
/// pointer to the entry data.
/// @param dir Directory handle, previously opened via
///        #VSSI_OpenDir().
/// @return Pointer to next directory entry. If no further entries, or dir is
///         not a valid open directory, returns NULL.
VSSI_Dirent* VSSI_ReadDir(VSSI_Dir dir);

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
///        #VSSI_OpenDir().
void VSSI_RewindDir(VSSI_Dir dir);

/// Reset the position for reading directory entries to the start of the
/// directory.
/// @param dir Directory handle, previously opened via
///        #VSSI_OpenDir2().
void VSSI_RewindDir2(VSSI_Dir2 dir);

/// Close a directory for an object, freeing local resources.
/// @param dir Directory handle, previously opened via
///        #VSSI_OpenDir().
void VSSI_CloseDir(VSSI_Dir dir);

/// Close a directory for an object, freeing local resources.
/// @param dir Directory handle, previously opened via
///        #VSSI_OpenDir2().
void VSSI_CloseDir2(VSSI_Dir2 dir);

/// Get a metadata value for a directory entry.
/// @param dirent Pointer to directory entry, provided by VSSI_ReadDir() 
/// @param type Metadata type desired. 
/// @return Pointer to metadata structure if found. If no such metadata is set
///         for the directory entry, returns NULL. 
VSSI_Metadata* VSSI_GetMetadataByType(VSSI_Dirent* dirent, u8 type);

/// Get the next unread metadata value for a directory entry.
/// @param dirent Pointer to directory entry, provided by VSSI_ReadDir() 
/// @return Pointer to the next unread metadata structure. If no metadata
///         remains, returns NULL. 
/// @note After calling VSSI_ReadDir() to get a directory entry, the next
///       metadata entry is the first one listed. 
VSSI_Metadata* VSSI_ReadMetadata(VSSI_Dirent* dirent);

/// Rewind metadata index to read the first entry on the next call to VSSI_ReadMetadata()
/// @param dirent Pointer to directory entry, provided by VSSI_ReadDir() 
void VSSI_RewindMetadata(VSSI_Dirent* dirent);

/// Read the next record from an opened trashcan for an object, returning a
/// pointer to the record data.
/// @param trashcan Trashcan handle, previously opened via 
///        #VSSI_OpenTrashcan().
/// @return Pointer to next trash record. If no further records, or trashcan is
///         not a valid trashcan handle, returns NULL.
VSSI_TrashRecord* VSSI_ReadTrashcan(VSSI_Trashcan trashcan);

/// Reset the position for reading trash records to the start of the
/// trashcan.
/// @param trashcan Trashcan handle, previously opened via
///        #VSSI_OpenTrashcan().
void VSSI_RewindTrashcan(VSSI_Trashcan trashcan);

/// Close a trashcan for an object, freeing local resources.
/// @param trashcan Trashcan handle, previously opened via
///        #VSSI_OpenTrashcan().
void VSSI_CloseTrashcan(VSSI_Trashcan trashcan);

//
// Session operations
//
// These operations make network connections, possibly reusing existing
// connections when possible. A valid user session is required.
//

/// Open a connection via proxy to a specific device.
/// @param session Handle to a previously registered Virtual Storage session.
/// @param proxy_addr Address (IP or hostname) of proxy server.
/// @param proxy_port Port of proxy server.
/// @param destination_device Device ID to connect to through the proxy server.
/// @param traffic_type Traffic type meaningful to destination device.
/// @param make_p2p Try to make a P2P connection. If P2P connect succeeds, return the P2P connection and close the proxy connection. Otherwise, return the proxy connection.
/// @param socket Pointer to location for the opened socket connection on success.
/// @param is_p2p If not NULL, pointer to location for flag indicating if the opened socket is a direct connection. When zero, the socket is via proxy. When nonzero, socket is direct (P2P connection after proxy worked).
/// @return Opened and connected socket if a connection via proxy server could be made.
/// @return Callback is called with the operation's result.
/// @note @a *socket will be filled-in with the opened TCP connection on
///       callback if successful. Otherwise, @a *socket will be VSSI_INVALID.
void VSSI_OpenProxiedConnection(VSSI_Session session,
                                const char* proxy_addr,
                                u16 proxy_port,
                                u64 destination_device,
                                u8 traffic_type,
                                int make_p2p,
                                VPLSocket_t* socket,
                                int *is_direct,
                                void* ctx,
                                VSSI_Callback callback);

/// DEPRECATED:  Use VSSI_OpenObject2 instead.
/// Open an object.
/// @param session Handle to a previously registered Virtual Storage session.
/// @param obj_xml XML string (NULL terminated) describing the object to open.
/// @param mode Object access mode. This is one of #VSSI_READONLY or
///              #VSSI_READWRITE. Writable files are always written with
///               overwrite mode, and created on open if not already existing.
/// @param handle Return pointer for object handle on success.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return Callback is called with the operation's result.
void VSSI_OpenObject(VSSI_Session session,
                     const char* obj_xml,
                     u8 mode,
                     VSSI_Object* handle,
                     void* ctx,
                     VSSI_Callback callback);

/// Open an object with directed route information.
/// @param session Handle to a previously registered Virtual Storage session.
/// @param user_id Owning user's ID
/// @param dataset_id ID of the dataset.
/// @param route_info Pointer to hand-crafted route information. 
/// @param mode Object access mode. This is one of #VSSI_READONLY or
///              #VSSI_READWRITE. Writable files are always written with
///               overwrite mode, and created on open if not already existing.
/// @param handle Return pointer for object handle on success.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return Callback is called with the operation's result.
void VSSI_OpenObject2(VSSI_Session session,
                      u64 user_id,
                      u64 dataset_id,
                      const VSSI_RouteInfo* route_info,
                      u8 mode,
                      VSSI_Object* handle,
                      void* ctx,
                      VSSI_Callback callback);

/// Delete an object.
/// @param session Handle to a previously registered virtual storage session.
/// @param obj_xml XML string (NULL terminated) describing the object to open.
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call.
/// @return Callback is called with the operation's result.
/// @note Deleting an object while it is open will still delete all data.
///        Only objects that could be written by a user can be deleted.
///        Once an object is deleted, it is lost for good on the server.
///        A new object with the same description may be created after
///        deletion, but it will be version 1 on first commit.
void VSSI_Delete(VSSI_Session session,
                 const char* obj_xml,
                 void* ctx,
                 VSSI_Callback callback);

/// Delete an object.
/// @param session Handle to a previously registered virtual storage session.
/// @param user_id Owning user's ID
/// @param dataset_id ID of the dataset.
/// @param route_info Pointer to hand-crafted route information. 
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call.
/// @return Callback is called with the operation's result.
/// @note Deleting an object while it is open will still delete all data.
///        Only objects that could be written by a user can be deleted.
///        Once an object is deleted, it is lost for good on the server.
///        A new object with the same description may be created after
///        deletion, but it will be version 1 on first commit.
void VSSI_Delete2(VSSI_Session session,
                  u64 user_id,
                  u64 dataset_id,
                  const VSSI_RouteInfo* route_info,
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

/// Clear version conflict for an object.
/// @param handle Handle for object with version conflict.
/// @return VSSI_SUCCESS when conflict cleared.
/// @note Use this to acknowledge a version conflict when detected.
///       The caller is responsible for cleaning up any data conflicts signaled
///       by the version conflict.
int VSSI_ClearConflict(VSSI_Object handle);

/// Send start_set command to provide the server a hint that a new change set
/// is about to be sent. This command should be used to cause any previously
/// abandoned change set for the object to be cleared. A newly opened object
/// would have no change log in-progress, making this command optional under
/// those conditions.
/// @param handle - Handle to the object being sending start_set.
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call.
/// @return Callback is called with the operation's result.
void VSSI_StartSet(VSSI_Object handle,
                   void* ctx,
                   VSSI_Callback callback);

/// Commit outstanding writes to an object. This commits all outstanding
/// changes to the object-files as an atomic action across
/// all object-files.
/// @param handle - Object handle
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call.
/// @return Callback is called with the operation's result.
void VSSI_Commit(VSSI_Object handle,
                 void* ctx,
                 VSSI_Callback callback);

/// Erase an object's contents.
/// @param handle - Object handle
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call.
/// @return Callback is called with the operation's result.
/// @note Erasing an object's data will reset its version to 0.
///        Object will not be readable until new data has been committed.
void VSSI_Erase(VSSI_Object handle,
                void* ctx,
                VSSI_Callback callback);

/// Read data from an object-file.
/// @param handle - Handle to the object being read
/// @param name - Object component (file) to read
/// @param offset - Read offset from beginning of file
/// @param length - Pointer to length of read requested as input parameter,
///                 and length actually read successfully as return value
/// @param buf - Pointer to buffer for read data, at least length bytes
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call.
/// @return Callback is called with the operation's result.
void VSSI_Read(VSSI_Object handle,
               const char* name,
               u64 offset,
               u32* length,
               char* buf,
               void* ctx,
               VSSI_Callback callback);

/// Read data from an object-file in a trashcan deletion record.
/// @param handle - Handle to the object being read
/// @param id - Trash record ID as read from a #VSSI_TrashRecord from #VSSI_ReadTrashcan.
/// @param name - Object component (file) to read
/// @param offset - Read offset from beginning of file
/// @param length - Pointer to length of read requested as input parameter,
///                 and length actually read successfully as return value
/// @param buf - Pointer to buffer for read data, at least length bytes
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call.
/// @return Callback is called with the operation's result.
void VSSI_ReadTrash(VSSI_Object handle,
                    VSSI_TrashId id,
                    const char* name,
                    u64 offset,
                    u32* length,
                    char* buf,
                    void* ctx,
                    VSSI_Callback callback);

/// Write data to an object asynchronously
/// @param handle Handle to the object being written
/// @param name Object component (file) to write
/// @param offset Write offset from beginning of file
/// @param length Pointer to length of write requested as input parameter,
///                 and length actually written successfully as return value
/// @param buf Pointer to buffer of write data, at least length bytes
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return Callback is called with the operation's result.
/// @note Writing beyond end-of-file will result in the file growing,
///       perhaps sparsely. Reading the space in-between writes will return
///       undefined data.
void VSSI_Write(VSSI_Object handle,
                const char* name,
                u64 offset,
                u32* length,
                const char* buf,
                void* ctx,
                VSSI_Callback callback);

/// Make a new directory component for this object.
/// @param object Handle to the object being changed.
/// @param name Directory to create.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @note All parent directories needed to make the named directory will be
///       created (like mkdir -p).
/// @note This command is only needed when creating empty directories.
/// @note Special directory names ".." and "." will be interpreted literally,
///       leading to failure.
/// @note This command will destroy existing files if they are in the way of
///       creating the desired directory.
void VSSI_MkDir(VSSI_Object handle,
                const char* name,
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
void VSSI_OpenDir(VSSI_Object handle,
                  const char* name,
                  VSSI_Dir* dir,
                  void* ctx,
                  VSSI_Callback callback);

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
void VSSI_Stat(VSSI_Object handle,
               const char* name,
               VSSI_Dirent** stats,
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

/// Open the trashcan of this object for reading.
/// @param object Handle to the object being accessed.
/// @param trashcan Pointer to #VSSI_Trashcan handle for subsequent reading from the open trashcan.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @notes The current trashcan contents will be read from the server with
///        this command. On success, @a trashcan may be used to read the
///        trashcan records.
void VSSI_OpenTrashcan(VSSI_Object handle,
                       VSSI_Trashcan* trashcan,
                       void* ctx,
                       VSSI_Callback callback);

/// Read the contents of a trash record directory, including contained
/// file/directory attributes and metadata.
/// @param object Handle to the object being accessed.
/// @param name Directory to open relative to the trash record.
/// @param id Trash record ID from #VSSI_ReadTrashcan returned #VSSI_TrashRecord.
/// @param dir Pointer to #VSSI_Dir handle for subsequent reading from the open
///        directory
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @notes Special directory names ".." and "." will be interpreted literally,
///        leading to failure.
///        The current directory information will be read from the server with
///        this command. On success, dir may be used to read the directory
///        entries.
void VSSI_OpenTrashDir(VSSI_Object handle,
                       VSSI_TrashId id,
                       const char* name,
                       VSSI_Dir* dir,
                       void* ctx,
                       VSSI_Callback callback);

/// Read the attributes and metadata for a single file or directory of a trash record.
/// @param object Handle to the object being accessed.
/// @param name File or directory to stat.
/// @param id Trash record ID from #VSSI_ReadTrashcan returned #VSSI_TrashRecord.
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
void VSSI_StatTrash(VSSI_Object handle,
                    VSSI_TrashId id,
                    const char* name,
                    VSSI_Dirent** stats,
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

/// Delete a trash record and all related data permanently.
/// @param object Handle to the object being changed.
/// @param id Trash record ID to delete.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
void VSSI_DeleteTrash(VSSI_Object handle,
                      VSSI_TrashId id,
                      void* ctx,
                      VSSI_Callback callback);

/// Delete all trash records and related data permanently.
/// @param object Handle to the object being changed.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
void VSSI_EmptyTrash(VSSI_Object handle,
                     void* ctx,
                     VSSI_Callback callback);

/// Rename/move a file or directory component.
/// @param handle Handle to the object being changed.
/// @param name File or directory to rename.
/// @param new_name New name for file or directory.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return #VSSI_INVALID if @a new_name contains a path prefix that matches
///         @a name, or vice-versa.
/// @note This operation functions like "rename(@a name, @a new_name)".
///       If @a name does not exist, this operation has no effect.
void VSSI_Rename(VSSI_Object object,
                 const char* name,
                 const char* new_name,
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

/// Copy a file or directory component.
/// @param handle Handle to the object being changed.
/// @param source File or directory to copy.
/// @param destination Name of copy to make.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return #VSSI_INVALID if trying to copy a component inside itself.
/// @notes This operation functions like "copy(@a source, @a destination)".
///        If @a source does not exist, this operation has no effect.
///        Any files in the path of @a destination will be destroyed to make
///        way @a destination.
///        Any file or directory at @a destination will be destroyed to make
///        way for @a destination.
void VSSI_Copy(VSSI_Object handle,
               const char* source,
               const char* destination,
               void* ctx,
               VSSI_Callback callback);

/// Copy a matching file (by signature and size) to a new file.
/// @param handle Handle to the object being changed.
/// @param size Size of the file to copy in bytes.
/// @param signature SHA-1 hash of the file to copy (20 bytes).
/// @param destination Name of copy to make.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return #VSSI_NOTFOUND if no matching file found to copy. Upload the file.
void VSSI_CopyMatch(VSSI_Object handle,
                    u64 size,
                    const char* signature,
                    const char* destination,
                    void* ctx,
                    VSSI_Callback callback);

/// Restore trashed data to the dataset.
/// @param handle Handle to the object being changed.
/// @param id Trash record ID to restore.
/// @param destination. Optional new name for the restored data.
///        If NULL, the original name of the data will be used.
///        If a zero-length string, the data will be restored as the entire
///        object contents.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @note If record @a id does not exist, this operation has no effect.
/// @note Any files in the path of @a destination (or the original name, if
///        destination is NULL) will be destroyed to make way for the
///        restored record.
/// @note Any file or directory at @a destination (or the original name, if
///        destination is NULL) will be destroyed to make way for the
///        restored record.
void VSSI_RestoreTrash(VSSI_Object handle,
                       VSSI_TrashId id,
                       const char* destination,
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

/// Set the size of an object-file, usually to truncate it.
/// @param object Handle to the object being written
/// @param name File to truncate.
/// @param size New size of the file. Bytes beyond the new size are lost.
///        If the size is increased, the new bytes are set to 0x00.
/// @param ctx Callback context on operation complete.
/// @param callback The callback function to call.
/// @return Callback is called with the operation's result.
/// @note This command will create the named file if it doesn't exist, even
///       destroying existing directories to do so.
void VSSI_Truncate(VSSI_Object handle,
                   const char* name,
                   u64 length,
                   void* ctx,
                   VSSI_Callback callback);

/// Set a metadata field for a component.
/// @param handle Handle to the object being changed 
/// @param name File or directory to modify, NULL terminated. 
/// @param type Type of metadata to set. 
/// @param length Length of metadata (0-255 bytes). 
/// @param data Metadata value to set. 
/// @param ctx Callback context on operation complete. 
/// @param callback The callback function to call. 
/// @note If name does not exist, a zero-byte file will be created.
void VSSI_SetMetadata(VSSI_Object handle,
                       const char* name,
                       u8 type,
                       u8 length,
                       const char* data,
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

///
/// VSSI Secure tunnel APIs.
///
/// Use these APIs to have the VSSI library create a secure connection to a 
/// personal storage server. 
///

/// Make a secure tunnel connection.
/// @param session Handle to a previously registered Virtual Storage session.
/// @param server_name Name or IP of server. If direct, the actual destination
///        name or IP. If proxy or P2P, proxy server facilitating connection.
/// @param server_port Port to use to connect to server.
/// @param connection_type Type of connection to make: Direct, Proxy or 
///        P2P upgrade after proxy.
/// @param destination_device Device ID to connect to through the proxy server.
/// @param tunnel_handle Handle to established tunnel, to be filled-in on
///        success before the callback is called.
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call.
/// @return Callback is called with the operation's result.
/// @note On success, the first listed path that successfully authenticates as
///       connected to the @a destination_device is used. if all paths fail to
///       connect or authenticate, VSSI_COMM is returned to indicate no secure
///       tunnel established.
void VSSI_SecureTunnelConnect(VSSI_Session session,
                              const char* server_name,
                              u16 server_port,
                              VSSI_SecureTunnelConnectType connection_type,
                              u64 destination_device,
                              VSSI_SecureTunnel* tunnel_handle,
                              void* ctx,
                              VSSI_Callback callback);

/// Break a secure tunnel connection made via #VSSI_SecureTunnelConnect().
/// @param tunnel_handle Handle to secure tunnel opened via 
///        #VSSI_SecureTunnelConnect().
void VSSI_SecureTunnelDisconnect(VSSI_SecureTunnel tunnel_handle);

/// Find out if the secure tunnel connection is direct or via proxy server.
/// @param tunnel_handle Handle to secure tunnel opened via 
///        #VSSI_SecureTunnelConnect().
/// @return false if connection goes through a proxy server.
/// @return false if tunnel not connected.
/// @return true if connection is direct. Direct connection may be because
///         tunnel was connected directly or via P2P connection after proxy 
///         connection made.
int VSSI_SecureTunnelIsDirect(VSSI_SecureTunnel tunnel_handle);
    
/// Send some data via a secure tunnel.
/// @param tunnel_handle Handle to secure tunnel opened via 
///        #VSSI_SecureTunnelConnect().
/// @param data Pointer to the data to send.
/// @param len Length of the data, in bytes.
/// @return Number of bytes sent on success.
/// @return #VSSI_INVALID if data is NULL or length is 0.
/// @return #VSSI_AGAIN if no further data queued to send and send buffer full.
///         Wait for data to be sent.
/// @return #VSSI_COMM if secure tunnel connection lost.
/// @note Data sent is put into the tunnel's send queue to be sent as allowed
///       by the underlying TCP connection. Success by this function only
///       indicates the data was queued for send and will be sent as the
///       connection allows. #VSSI_HandleSockets() must be called to cause data
///       to actually be sent.
VSSI_Result VSSI_SecureTunnelSend(VSSI_SecureTunnel tunnel_handle,
                                  const char* data,
                                  size_t len);

/// Request callback when ready to send more data.
/// @param tunnel_handle Handle to secure tunnel opened via 
///        #VSSI_SecureTunnelConnect().
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call
/// @return Callback is called with the operation's result.
/// @return #VSSI_SUCCESS if able to send at least one byte immediately.
/// @return #VSSI_COMM if secure tunnel connection lost.
void VSSI_SecureTunnelWaitToSend(VSSI_SecureTunnel tunnel_handle,
                                 void* ctx,
                                 VSSI_Callback callback);

/// Receive some data via a secure tunnel.
/// @param tunnel_handle Handle to secure tunnel opened via 
///        #VSSI_SecureTunnelConnect().
/// @param data Pointer to the data to send.
/// @param len Length of the data, in bytes.
/// @return Number of bytes sent on success.
/// @return #VSSI_INVALID if data is NULL or length is 0.
/// @return #VSSI_AGAIN if no data available now but connection is still open.
/// @return #VSSI_COMM if secure tunnel connection lost.
/// @note Data returned comes from the receive buffer for the secure tunnel. 
///       #VSSI_HandleSockets() must be called to cause data to be received and
///       placed into the receive buffer after authentication.
VSSI_Result VSSI_SecureTunnelReceive(VSSI_SecureTunnel tunnel_handle,
                                     char* data,
                                     size_t len);

/// Request callback when data is ready to be received.
/// @param tunnel_handle Handle to secure tunnel opened via 
///        #VSSI_SecureTunnelConnect().
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call
/// @return Callback is called with the operation's result.
/// @return #VSSI_SUCCESS if data is ready to be received.
/// @return #VSSI_COMM if secure tunnel connection lost.
void VSSI_SecureTunnelWaitToReceive(VSSI_SecureTunnel tunnel_handle,
                                    void* ctx,
                                    VSSI_Callback callback);

/// Request a secure tunnel state be reset, flushing all bytes received and
/// pending send. Callback will be called if an error occurs or when the other
/// end of the connection acknowledges that the connection has been reset.
/// @param tunnel_handle Handle to secure tunnel opened via 
///        #VSSI_SecureTunnelConnect().
/// @param ctx - Callback context on operation complete.
/// @param callback - The callback function to call
/// @return Callback is called with the operation's result.
/// @return #VSSI_SUCCESS if connection is reset successfully.
/// @return #VSSI_COMM if secure tunnel connection lost.
/// @notes On reset, both send and receive buffers are flushed.
///        #VSSI_SecureTunnelSend() and #VSSI_SecureTunnelReceive() will return
///        #VSSI_AGAIN until reset is complete and ready for send/receive.
void VSSI_SecureTunnelReset(VSSI_SecureTunnel tunnel_handle,
                            void* ctx,
                            VSSI_Callback callback);

#ifdef __cplusplus
}
#endif

#endif // include guard
