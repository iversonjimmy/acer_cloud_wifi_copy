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

#ifndef STORAGE_NODE__VSS_SESSION_HPP__
#define STORAGE_NODE__VSS_SESSION_HPP__

class vss_session;

#include "vpl_types.h"
#include "vpl_time.h"

#include <map>
#include <string>

#include "vss_object.hpp"
#include "dataset.hpp"
#include "xid_sequence.hpp"
#include "vss_query.hpp"

#include "csltypes.h"

/// Sub-sessions keyed by device ID, allowing one login session to be
/// used by many devices.
class vss_sub_session
{
private:
    VPL_DISABLE_COPY_AND_ASSIGN(vss_sub_session);

public:
    vss_sub_session(u64 device_id,
                    u64 session_handle,
                    const std::string& ds_service_ticket);
    ~vss_sub_session() {};
    
    /// Device ID for this session
    u64 device_id;

    char encryption_key[CSL_AES_KEYSIZE_BYTES];
    char signing_key[CSL_SHA1_DIGESTSIZE];

    u8 signing_mode;
    u8 sign_type;
    u8 encrypt_type;
    std::string challenge;
    
    /// Session sequence state, for continuing session replay defense.
    xid_sequence sequence_state;
};

/// A single user session context.
/// This object contains the server-persistent state for a given user's 
/// session (where session is loosely defined as a login session).
/// This session context is filled-in with user information as it becomes
/// needed. Any user query is formed from the data within the session.
class vss_session {
public:
    vss_query* query;

protected:
    std::map<u64, vss_sub_session*> sub_sessions;
    vss_sub_session* get_sub_session(u64 device_id);

    // Collection of open storage objects (Index = handle)
    std::map<u32, vss_object*> objects;

    // Hint for next object handle to use
    u32 object_handle_hint;

    VPLMutex_t mutex;

 public:
    vss_session(u64 handle, u64 uid,
                const std::string& serviceTicket,
                vss_query& query);
    ~vss_session();

    // Get next XID acceptable to a sub-session.
    u32 get_next_sub_session_xid(u64 device_id);

    u64 get_session_handle() 
    { 
        return session_handle;
    }

    const std::string& get_service_ticket()
    {
        return serviceTicket;
    }

    u64 get_uid()
    {
        return uid;
    }

    // Returns true if input settings are compatible for this session.
    // * New session and no challenge provided (takes security settings, randomizes XID)
    // * Existing session, no challenge, and security settings match (XID unchanged)
    // * Existing session and challenge matches (takes security settings, resets XID)
    // Otherwise a challenge is generated and returns false. Client must retry.
    bool test_security_settings(u64 device_id,
                                u8& signing_mode, u8& sign_type, u8& encrypt_type,
                                std::string& challenge);

    VPLTime_t last_updated;

    /// Determine if a request header is valid for this session.
    /// @param req - Pointer to command buffer.
    ///              Buffer is at least VSCS_HEADER_SIZE bytes.
    /// @return BVS_SUCCESS if request is validated and in-sequence.
    ///         -BVS_BADXID if request has a bad XID.
    ///         -BVS_PERM if request signature fails to match (permission
    ///                   denied for processing)
    int verify_header(const char* req);
    int verify_header(const char* req,
                      u8 signing_mode,
                      u8 sign_type);

    /// Determine if a request header is valid for this session.
    /// @param req - Pointer to command buffer.
    ///              Buffer is at least VSCS_HEADER_SIZE bytes.
    /// @return BVS_SUCCESS if request is validated and in-sequence.
    ///         -BVS_BADXID if request has a bad XID.
    ///         -BVS_PERM if request signature fails to match (permission
    ///                   denied for processing)
    /// @return verify_device - whether the device ID should be white-listed
    int verify_header(const char* req, bool& verify_device);
    int verify_header(const char* req, bool& verify_device,
                      u8 signing_mode,
                      u8 sign_type);

    /// Determine if a request (its data) is valid for this session.
    /// @param req - Pointer to request buffer.
    ///              Buffer has a data length field, data signature.
    /// @return BVS_SUCCESS if request is validated and in-sequence.
    ///         -BVS_PERM if request signature fails to match (permission
    ///                   denied for processing)
    int validate_request_data(char* req);
    int validate_request_data(char* req,
                              u8 signing_mode,
                              u8 sign_type,
                              u8 encrypt_type);
    int validate_request_data(char* header, char* data);
    int validate_request_data(char* header, char* data,
                              u8 signing_mode,
                              u8 sign_type,
                              u8 encrypt_type);

    /// Sign a reply buffer with this session's signing key
    /// @param reply - Pointer to the reply buffer. Must be contiguous memory.
    ///                  Buffer has a total length field.
    /// @side-effect Space for the reply signature is filled-in with the
    ///              computed signature value.
    void sign_reply(char*& reply);
    void sign_reply(char*& reply,
                    u8 signing_mode,
                    u8 sign_type,
                    u8 encrypt_type);

    // Create a new object under this session.
    // The object will have a unique handle for this session.
    // Object reference count is set to 1 on creation.
    vss_object* new_object(dataset* dataset,
                           u64 origin_device,
                           u64 user_id,
                           u32 flags);

    // Find an existing object (do not create a new one)
    // Return pointer to object if it exists, and increase reference count.
    // Returns NULL if no such object.
    // If doAccess is true, change the last client access time.
    vss_object* find_object(u32 handle, bool doAccess = true);

    // Find existing objects by user id and dataset id (do not create a new one)
    void find_object_handles(u64 uid, u64 did, std::vector<u32>& handle_list);

    // Try to clear timeout objects
    bool try_clear_timeout_objects();

    // Reduce reference count to an existing object.
    // If reference count is taken to zero, delete the object.
    void release_object(u32 handle);

 private:
    VPL_DISABLE_COPY_AND_ASSIGN(vss_session);
    
    // User ID, for validating save state access
    u64 uid;

    /// Session handle, for continuing sessions
    u64 session_handle;

    /// Session service ticket, for continuing sessions
    std::string serviceTicket;

    /// Device specific service ticket
    std::string dsServiceTicket;

    // Random pad generation
    u32 randomness;
    u16 get_randomness()
    {
        randomness = randomness * 1103515245 + 12345;
        return randomness & 0xffff;
    }

    // Remove an object from object list if its refcount is zero.
    // @param handle, handle of object to be removed from object list.
    // @param object, pointer to an object that has been removed from the object list.
    void release_object_unlocked(u32 handle, vss_object*& object);
};


#endif // include guard
