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

#ifndef __VSSI_VSS_H__
#define __VSSI_VSS_H__

#include "vssi_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default VSS port
#define VSS_SERVER_PORT 16957

/// Version 0: Sign with service ticket only.
/// Version 1: Sign with derived ticket. Encrypt body data before sign.
/// Use 1 for added security.
#define USE_VSSI_VERSION 2

/// Send proxy connection request.
VSSI_Result VSSI_SendProxyRequest(VSSI_UserSession* session,
                                  u64 destination_device,
                                  u8 traffic_type,
                                  VPLNet_port_t p2p_port,
                                  VPLSocket_t connection);

/// Send command ops
/// These ops send the named command, composed from information in the
/// provided arguments.

/// Open - Send an open object command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendOpenCommand(VSSI_ObjectState* object,
                          VSSI_PendInfo* info);

/// Close - Send an object close command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendCloseCommand(VSSI_ObjectState* object,
                           VSSI_PendInfo* info);

/// Read - Send object-file read command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Object-file name for read
/// @param offset Read offset from beginning of file
/// @param length Length of read requested
void VSSI_SendReadCommand(VSSI_ObjectState* object,
                          VSSI_PendInfo* info,
                          const char* name,
                          u64 offset,
                          u32 length);

/// Read - Send trash object-file read command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param id Trash record ID for read
/// @param name Object-file name for read
/// @param offset Read offset from beginning of file
/// @param length Length of read requested
void VSSI_SendReadTrashCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               VSSI_TrashId id,
                               const char* name,
                               u64 offset,
                               u32 length);
    
/// Write - Send object-file write command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Object-file name for read
/// @param offset Write offset from beginning of file
/// @param length Length of write requested
/// @param buf Pointer to buffer of write data, at least length bytes
void VSSI_SendWriteCommand(VSSI_ObjectState* object,
                           VSSI_PendInfo* info,
                           const char* name,
                           u64 offset,
                           u32 length,
                           const char* buf);

/// Set Times - Set the timetamps of a component.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Object-file to change.
/// @param ctime New creation time, unless 0.
/// @param mtime New modification time, unless 0.
void VSSI_SendSetTimesCommand(VSSI_ObjectState* object,
                              VSSI_PendInfo* info,
                              const char* name,
                              VPLTime_t ctime, 
                              VPLTime_t mtime);


/// Set Size - Set the size of a file, usually to truncate it.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Object-file to change.
/// @param length New size of the file.
void VSSI_SendSetSizeCommand(VSSI_ObjectState* object,
                             VSSI_PendInfo* info,
                             const char* name,
                             u64 length);

/// Set Metadata - Set some component metadata.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Object-file/directory to change.
/// @param type Type of metadata value being set.
/// @param length Length of metadata value being set.
/// @param data Pointer to metadata value, @a length bytes.
void VSSI_SendSetMetadataCommand(VSSI_ObjectState* object,
                                 VSSI_PendInfo* info,
                                 const char* name,
                                 u8 type,
                                 u8 length,
                                 const char* data);

/// Commit - Send an object commit command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendCommitCommand(VSSI_ObjectState* object,
                            VSSI_PendInfo* info);

/// Commit - Send an object erase command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendEraseCommand(VSSI_ObjectState* object,
                           VSSI_PendInfo* info);

/// Start set - Send an object start change set command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendStartSetCommand(VSSI_ObjectState* object,
                              VSSI_PendInfo* info);
    
/// Delete - Send an object delete command
/// @param object Temp object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendDeleteCommand(VSSI_ObjectState* object,
                            VSSI_PendInfo* info);

/// Remove - Send a component remove command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Component name, NULL terminated.
void VSSI_SendRemoveCommand(VSSI_ObjectState* object,
                            VSSI_PendInfo* info,
                            const char* name);

/// Rename - Rename a component, replacing whatever exists at the new name.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Component name, NULL terminated.
/// @param new_name New component name, NULL terminated.
void VSSI_SendRenameCommand(VSSI_ObjectState* object,
                            VSSI_PendInfo* info,
                            const char* name,
                            const char* new_name);

void VSSI_SendRename2Command(VSSI_ObjectState* object,
                            VSSI_PendInfo* info,
                            const char* name,
                            const char* new_name,
                            u32 flags);

/// Copy - Copy a component, replacing whatever exists at the destination.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param source Source component name, NULL terminated.
/// @param destination Destination component name, NULL terminated.
void VSSI_SendCopyCommand(VSSI_ObjectState* object,
                          VSSI_PendInfo* info,
                          const char* source,
                          const char* destination);

/// CopyMatch - Copy a component by size and signature reference.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param size Size of matching file to copy.
/// @param signature Signature of matching file to copy (20 bytes).
/// @param destination Destination component name, NULL terminated.
void VSSI_SendCopyMatchCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               u64 size,
                               const char* signature,
                               const char* destination);

/// Read Directory - Send a read directory command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Component name, NULL terminated.
void VSSI_SendReadDirCommand(VSSI_ObjectState* object,
                             VSSI_PendInfo* info,
                             const char* name);

void VSSI_SendReadDir2Command(VSSI_ObjectState* object,
                              VSSI_PendInfo* info,
                              const char* name);

/// Read Trash Directory - Send a read trash directory command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param id Trash record ID.
/// @param name Component name, NULL terminated.
void VSSI_SendReadTrashDirCommand(VSSI_ObjectState* object,
                                  VSSI_PendInfo* info,
                                  VSSI_TrashId id,
                                  const char* name);

/// Stat - Send a file/directory stat command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Component name, NULL terminated.
void VSSI_SendStatCommand(VSSI_ObjectState* object,
                          VSSI_PendInfo* info,
                          const char* name);

/// Stat2 - Send a file/directory stat command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Component name, NULL terminated.
void VSSI_SendStat2Command(VSSI_ObjectState* object,
                           VSSI_PendInfo* info,
                           const char* name);

/// Stat Trash - Send a trash file/directory stat command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param id Trash record ID.
/// @param name Component name, NULL terminated.
void VSSI_SendStatTrashCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               VSSI_TrashId id,
                               const char* name);

/// Read Trashcan - Send a read trashcan command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendReadTrashcanCommand(VSSI_ObjectState* object,
                                  VSSI_PendInfo* info);

/// Make Directory - Send a make directory command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name directory name, NULL terminated.
void VSSI_SendMakeDirCommand(VSSI_ObjectState* object,
                             VSSI_PendInfo* info,
                             const char* name);

/// Make Directory - Send a make directory command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name directory name, NULL terminated.
/// @param attrs directory attributes
void VSSI_SendMakeDir2Command(VSSI_ObjectState* object,
                              VSSI_PendInfo* info,
                              const char* name,
                              u32 attrs);

/// Chmod - Change attributes of file or directory
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name file/directory name, NULL terminated.
/// @param attrs attributes
/// @param attrs_mask attributes (for a read-mod-write op)
void VSSI_SendChmodCommand(VSSI_ObjectState* object,
                           VSSI_PendInfo* info,
                           const char* name,
                           u32 attrs,
                           u32 attrs_mask);

/// Empty Trash - Send empty trash command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendEmptyTrashCommand(VSSI_ObjectState* object,
                                VSSI_PendInfo* info);

/// Delete Trash - Send delete trash record command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param id Trash record to delete.
void VSSI_SendDeleteTrashCommand(VSSI_ObjectState* object,
                                 VSSI_PendInfo* info,
                                 VSSI_TrashId id);

/// Restore Trash - Send restore from trash command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param id Trash record to delete.
/// @param name (optional) pointer to name to used for restored data.
void VSSI_SendRestoreTrashCommand(VSSI_ObjectState* object,
                                  VSSI_PendInfo* info,
                                  VSSI_TrashId id,
                                  const char* name);

/// Get Space - Get disk space information.
/// @param object Temp object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendGetSpaceCommand(VSSI_ObjectState* object,
                              VSSI_PendInfo* info);

/// Get Notify events - Send get notify command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendGetNotifyCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* cmd_info);

/// Set Notify events - Send set notify command.
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param mask Notify events mask to set.
void VSSI_SendSetNotifyCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* cmd_info,
                               VSSI_NotifyMask mask);

/// Get File Lock State - Send get lock command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Object-file name for which to get lock state
void VSSI_SendGetLockCommand(VSSI_ObjectState* object,
                             VSSI_PendInfo* info);

/// Set File Lock - Send set lock command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Object-file name on which to set lock
/// @param lock_mask value to set for file lock mask
void VSSI_SendSetLockCommand(VSSI_ObjectState* object,
                             VSSI_PendInfo* info,
                             u64 lock_mask);

void VSSI_SendSetLockRangeCommand(VSSI_ObjectState* object,
                                  VSSI_PendInfo* info,
                                  VSSI_ByteRangeLock* br_lock,
                                  u32 flags);

/// Open File - Send open file command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
/// @param name Object-file name to open
/// @param flags value for open file
void VSSI_SendOpenFileCommand(VSSI_ObjectState* object,
                              VSSI_PendInfo* info,
                              const char* name,
                              u32 flags,
                              u32 attrs);

void VSSI_SendReadFileCommand(VSSI_ObjectState* object,
                              VSSI_PendInfo* info,
                              u64 offset,
                              u32 length);

void VSSI_SendWriteFileCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               u64 offset,
                               u32 length,
                               const char* buf);

void VSSI_SendTruncateFileCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               u64 offset);

void VSSI_SendChmodFileCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               u32 attrs,
                               u32 attrs_mask);

/// Release File - Send a file handle release command - just release file locks without closing
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendReleaseFileCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info);

/// Close File - Send a file handle close command
/// @param object Object pointer, used to get session & connection
/// @param info Pointer to command info, containing callback and instructions for command resolution.
void VSSI_SendCloseFileCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info);

/// Send a VSSI message.
/// If needed, a connection to the destination will be made and security parameters negotiated
/// before the message is sent.
/// The message sent will be the concatenation of 
/// @a header | @a fixed_data | @a var_data1 | @a var_data2 
/// with signing, encrypting, and padding performed as necessary.
void VSSI_SendVss(VSSI_ObjectState* object,
                  VSSI_PendInfo* cmd_info,
                  char* header,
                  const char* fixed_data, size_t fixed_len,
                  const char* var_data1, size_t var_len1,
                  const char* var_data2, size_t var_len2);


/// Prepare message (allocate single buffer with padding).
VSSI_MsgBuffer* VSSI_PrepareMessage(char* header,
                                    const char* fixed_data, size_t fixed_len,
                                    const char* var_data1, size_t var_len1,
                                    const char* var_data2, size_t var_len2);

/// Compose message (apply security measures).
VSSI_MsgBuffer* VSSI_ComposeMessage(VSSI_UserSession* session,
                                    u8 sign_mode, u8 sign_type, u8 enc_type,
                                    VSSI_MsgBuffer* msg);

/// Send a message to a server via server connection.
/// A connected socket to the server will transmit the message.
void VSSI_SendToServer(VSSI_VSConnectionNode* server_connection, 
                       VSSI_PendInfo* cmd_info);
/// Same, but assume lock already held.
void VSSI_SendToServerUnlocked(VSSI_VSConnectionNode* server_connection, 
                               VSSI_PendInfo* cmd_info);

/// Negotiate session parameters for a user session and server connection for an object.
VSSI_Result VSSI_NegotiateSession(VSSI_ObjectState* object,
                                  VSSI_SendState* send_state,
                                  const char* challenge, u8 len);
void VSSI_NegotiateSessionDone(VSSI_UserSession* session,
                               VSSI_SendState* send_state,
                               VSSI_VSConnectionNode* server_connection,
                               const char* msg);

/// Handle command APIs
/// These functions handle replies to the named commands, parsing the reply
/// and returning information in the provided arguments.
/// All of them return the reply return code.

/// Verify an incoming header's signature.
/// If the header has a bad signature or the user session is not valid,
/// the data length cannot be trusted.
/// Treat as a lost connection.
VSSI_Result VSSI_VerifyHeader(char* header,
                              VSSI_UserSession* session,
                              u8 sign_mode, u8 sign_type);

/// Verify an incoming command's data.
/// If the data has a bad signature, the command cannot be trusted.
VSSI_Result VSSI_VerifyData(char* command, size_t cmdlen,
                            VSSI_UserSession* session,
                            u8 sign_mode, u8 sign_type, u8 enc_type);

/// Error - Just return the command result.
VSSI_Result VSSI_HandleErrorReply(char* reply);

/// Version-only response - Just update the version and return command result.
VSSI_Result VSSI_HandleVersionReply(char* reply,
                                    VSSI_ObjectState* object);


/// Open - Fill in object with information from reply on success.
/// @param object - Object being opened
VSSI_Result VSSI_HandleOpenReply(char* reply,
                                 VSSI_ObjectState* object);

/// Close - Just return the command result.
VSSI_Result VSSI_HandleCloseReply(char* reply,
                                  VSSI_ObjectState* object);

/// Read - Fill in buffer, length read, object version on success.
/// @param [in] object - Object pointer
/// @param [out] length - Length of read performed on success
/// @param [out] buf - Buffer to receive successfully read data.
VSSI_Result VSSI_HandleReadReply(char* reply,
                                 VSSI_ObjectState* object,
                                 u32* length,
                                 char* buf);

/// Write - Fill in length read, object version on success.
/// @param [in] object - Object pointer
/// @param [out] length - Length of read performed on success
VSSI_Result VSSI_HandleWriteReply(char* reply,
                                  VSSI_ObjectState* object,
                                  u32* length);

/// Delete - Just return the command result.
VSSI_Result VSSI_HandleDeleteReply(char* reply);

/// Make Directory - Just return the command result.
VSSI_Result VSSI_HandleMakeDirReply(char* reply,
                                    VSSI_ObjectState* object);

/// Remove - Just return the command result.
VSSI_Result VSSI_HandleRemoveReply(char* reply,
                                   VSSI_ObjectState* object);

/// Rename - Just return the command result.
VSSI_Result VSSI_HandleRenameReply(char* reply,
                                   VSSI_ObjectState* object);

/// Read Directory - Put gathered data into the directory struct.
VSSI_Result VSSI_HandleReadDirReply(char* reply,
                                    VSSI_ObjectState* object,
                                    VSSI_Dir* directory,
                                    int version);

/// Stat - Put gathered data into the Stat struct.
VSSI_Result VSSI_HandleStatReply(char* reply,
                                 VSSI_ObjectState* object,
                                 VSSI_Dirent** stats);

/// Stat - Put gathered data into the Stat struct.
VSSI_Result VSSI_HandleStat2Reply(char* reply,
                                  VSSI_ObjectState* object,
                                  VSSI_Dirent2** stats);

/// Read trashcan - Put gathered data into the trashcan struct.
VSSI_Result VSSI_HandleReadTrashcanReply(char* reply,
                                         VSSI_ObjectState* object,
                                         VSSI_Trashcan* trashcan);

/// Get Space - Get disk space information.
VSSI_Result VSSI_HandleGetSpaceReply(char* reply,
                                     VSSI_ObjectState* object,
                                     u64* disk_size,
                                     u64* dataset_size,
                                     u64* avail_size);

/// Open File - Fill in file object with information from reply on success.
/// @param file_state - file handle being created
VSSI_Result VSSI_HandleOpenFileReply(char* reply,
                                     VSSI_FileState* file_state);

/// CloseFile - Fill in object with information from reply on success.
/// @param object - Object being closed
/// @param delete_filestate_when - Flag for cleanup in VSSI_ResolveCommand
VSSI_Result VSSI_HandleCloseFileReply(char* reply,
                                    VSSI_ObjectState* object,
                                    int* delete_filestate_when);

/// Read File - Return length read and copy data on success.
VSSI_Result VSSI_HandleReadFileReply(char* reply,
                                     u32* length,
                                     char* buf);

/// Write File - Return length written on success.
VSSI_Result VSSI_HandleWriteFileReply(char* reply,
                                     u32* length);

/// Truncate File
VSSI_Result VSSI_HandleTruncateFileReply(char* reply);

/// Chmod File
VSSI_Result VSSI_HandleChmodFileReply(char* reply);

/// Set File Lock State
VSSI_Result VSSI_HandleSetFileLockReply(char* reply);

/// Get File Lock State
VSSI_Result VSSI_HandleGetFileLockReply(char* reply,
                                        VSSI_FileLockState* lock_state);

/// Set Byte Range Lock
VSSI_Result VSSI_HandleSetLockRangeReply(char* reply);

/// Get/Set notify events mask reply - Put mask at location provided.
VSSI_Result VSSI_HandleGetNotifyReply(char* reply,
                                      VSSI_NotifyMask* mask_out);


/// Compute sha1 HMAC for some data. Up to 2 data blocks may be signed together.
/// @param buf, Pointer data blocks to sign.
/// @param len, The data block length
/// @param key Pointer to key material. Must be CSL_SHA1_DIGESTSIZE(20) bytes.
/// @param hmac Pointer to buffer to place resulting HMAC value. 
/// @param hmac_len Number of leading bytes of the HMAC needed. Must be <= 20.
void compute_hmac(const char* buf, size_t len,
                  const void* key, char* hmac, int hmac_size);
/// Encrypt/decrypt data.
void encrypt_data(char* dest, const char* src, size_t len, 
                  const char* iv_seed, const char* key);
void decrypt_data(char* dest, const char* src, size_t len, 
                  const char* iv_seed, const char* key);

#ifdef __cplusplus
}
#endif

#endif // include guard
