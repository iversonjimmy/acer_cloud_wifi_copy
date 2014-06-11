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

#include "vplex_trace.h"

#include "cslsha.h" // SHA1 HMAC
#include "aes.h"    // AES128 encrypt/decrypt

#include "vss_comm.h"
#include "vssi_error.h"

#include "vss_session.hpp"

#include <sstream>
#include <iomanip>

using namespace std;

vss_session::vss_session(u64 handle, u64 uid,
                         const std::string& serviceTicket,
                         vss_query& query) :
    uid(uid),
    session_handle(handle),
    serviceTicket(serviceTicket)
{
    VPLMutex_Init(&mutex);
    last_updated = VPLTime_GetTimeStamp();
    randomness = last_updated & 0xffffffff;
    object_handle_hint = VPLTime_GetTime() & 0xffffffff;
    this->query = &query;
}

vss_session::~vss_session()
{
    std::map<u64, vss_sub_session*>::iterator it;
    map<u32, vss_object*>::iterator o_it;

    while(!sub_sessions.empty()) {
        it = sub_sessions.begin();
        delete it->second;
        sub_sessions.erase(it);
    }

    for(o_it = objects.begin(); o_it != objects.end(); o_it++) {
        vss_object* object = o_it->second;
        // How would this be NULL?
        if(object == NULL) {
            continue;
        }
        if(!object->is_closed()) {
            object->close();
        }
        if(object->get_refcount() != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "object "FMTu32" still referenced?",
                object->get_handle());
        }
        delete object;
    }
    objects.clear();
    VPLMutex_Destroy(&mutex);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Removing session with handle %"PRIx64,
                      session_handle);            
}

bool vss_session::test_security_settings(u64 device_id,
                                         u8& signing_mode, u8& sign_type, u8& encrypt_type,
                                         std::string& challenge)
{
    bool rv = false;
    vss_sub_session* sub_session = get_sub_session(device_id);
    if ( sub_session == NULL ) {
        goto exit;
    }

    VPLMutex_Lock(&mutex);

    // Returns true if input settings are compatible for this session.
    // * New session and no challenge provided (takes security settings, randomizes XID)
    // * Existing session, no challenge, and security settings match (XID unchanged)
    // * Existing session and challenge matches (takes security settings, resets XID)
    if(challenge.empty()) {
        if(sub_session->sequence_state.is_fresh()) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Negotiate session "FMTu64" from "FMTu64" accepted for new state.",
                              session_handle, device_id);
            rv = true;

            // Set random start XID.
            u32 xid = 0;
            while(xid == 0) {
                xid = get_randomness() + (get_randomness() << 16);
            }
            sub_session->sequence_state.accept(xid);  

            // Set security settings as requested if on a trusted network.
            // Otherwise, use high security settings.
            if(query->isTrustedNetwork()) {                
                sub_session->signing_mode = signing_mode;
                sub_session->sign_type = sign_type;
                sub_session->encrypt_type = encrypt_type;
            }
            else {
                signing_mode = sub_session->signing_mode = VSS_NEGOTIATE_SIGNING_MODE_FULL;
                sign_type = sub_session->sign_type = VSS_NEGOTIATE_SIGN_TYPE_SHA1;
                encrypt_type = sub_session->encrypt_type = VSS_NEGOTIATE_ENCRYPT_TYPE_AES128;
            }
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Negotiate session "FMTu64" from "FMTu64" set security to %d:%d:%d.",
                              session_handle, device_id,
                              sub_session->signing_mode, sub_session->sign_type,
                              sub_session->encrypt_type);
        }
        else {
            if(sub_session->signing_mode == signing_mode &&
               sub_session->sign_type == sign_type &&
               sub_session->encrypt_type == encrypt_type) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Negotiate session "FMTu64" from "FMTu64" accepted for same security level as established.",
                                  session_handle, device_id);
                rv = true;
            }
        }
    }
    else {
        // Challenge provided. Must match or reissue challenge.
        if(challenge.compare(sub_session->challenge) == 0) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Negotiate session "FMTu64" challenge from "FMTu64" accepted.",
                              session_handle, device_id);

            // Get random XID.
            u32 xid = 0;
            while(xid == 0) {
                xid = get_randomness() + (get_randomness() << 16);
            }

            // Challenge-based negotiation resets session.
            sub_session->sequence_state.reset();
            sub_session->sequence_state.accept(xid);        

            // Set security settings as requested if on a trusted network.
            // Otherwise, use high security settings.
            if(query->isTrustedNetwork()) {                
                sub_session->signing_mode = signing_mode;
                sub_session->sign_type = sign_type;
                sub_session->encrypt_type = encrypt_type;
            }
            else {
                signing_mode = sub_session->signing_mode = VSS_NEGOTIATE_SIGNING_MODE_FULL;
                sign_type = sub_session->sign_type = VSS_NEGOTIATE_SIGN_TYPE_SHA1;
                encrypt_type = sub_session->encrypt_type = VSS_NEGOTIATE_ENCRYPT_TYPE_AES128;
            }
            
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Negotiate session "FMTu64" from "FMTu64" with challenge set security to %d:%d:%d.",
                              session_handle, device_id,
                              sub_session->signing_mode, sub_session->sign_type,
                              sub_session->encrypt_type);

            rv = true;
        }
        else {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Negotiate session "FMTu64" challenge from "FMTu64" mismatch. Issue new challenge.",
                              session_handle, device_id);
        }
    }

    if(!rv) {
        // Negotiation not successful. Compose new challenge.
        sub_session->challenge.clear();
        for(int i = 0; i < 16; i++) {
            char byte = (char)(get_randomness());
            sub_session->challenge += byte;
        }
        challenge = sub_session->challenge;
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Negotiate session "FMTu64" from "FMTu64" being challenged.",
                          session_handle, device_id);
    }

    VPLMutex_Unlock(&mutex);

 exit:
    return rv;
}

u32 vss_session::get_next_sub_session_xid(u64 device_id)
{
    u32 rv = 0;
    vss_sub_session* sub_session = get_sub_session(device_id);

    if ( sub_session ) {
        rv = sub_session->sequence_state.next_valid();
    }

    return rv;
}

static void compute_hmac(const char* buf, size_t len,
                         const void* key, char* hmac, int hmac_size)
{
    CSL_HmacContext context;
    char calc_hmac[CSL_SHA1_DIGESTSIZE] = {0};

    if(len <= 0) {
        // No signature for empty data. Return 0.
        memset(hmac, 0, hmac_size);
        return;
    }

    CSL_ResetHmac(&context, (u8*)key);
    CSL_InputHmac(&context, buf, len);
    CSL_ResultHmac(&context, (u8*)calc_hmac);

    if(hmac_size > CSL_SHA1_DIGESTSIZE) {
        hmac_size = CSL_SHA1_DIGESTSIZE;
    }
    memcpy(hmac, calc_hmac, hmac_size);
}

static void encrypt_data(char* dest, const char* src, size_t len, 
                         const char* iv_seed, const char* key)
{
    int rc;

    char iv[CSL_AES_IVSIZE_BYTES];
    memcpy(iv, iv_seed, sizeof(iv));

    rc = aes_SwEncrypt((u8*)key, (u8*)iv, (u8*)src, len, (u8*)dest);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to encrypt data.");
    }
}

static void decrypt_data(char* dest, const char* src, size_t len, 
                         const char* iv_seed, const char* key)
{
    int rc;

    char iv[CSL_AES_IVSIZE_BYTES];
    memcpy(iv, iv_seed, sizeof(iv));

    rc = aes_SwDecrypt((u8*)key, (u8*)iv, (u8*)src, len, (u8*)dest);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to decrypt data.");
    }
}

int vss_session::verify_header(const char* header, bool& verify_device,
                               u8 signing_mode,
                               u8 sign_type)
{
    int rv = VSSI_SUCCESS;
    char orig_hmac[VSS_HMAC_SIZE];
    char calc_hmac[VSS_HMAC_SIZE];
    vss_sub_session* subSession = get_sub_session(vss_get_device_id(header));

    // we only allow device specific tickets
    verify_device = true;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Verifying header for command %x, XID %u for session "FMTx64".",
                        vss_get_command(header), vss_get_xid(header),
                        vss_get_handle(header));

    if ( subSession == NULL ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Missing dev spec ticket.");
        rv = VSSI_BADSIG;
        goto exit;
    }
    
    // Verify header signature.
    memcpy(orig_hmac, vss_get_header_hmac(header), VSS_HMAC_SIZE);
    memset(vss_get_header_hmac(header), 0, VSS_HMAC_SIZE);

    switch(vss_get_version(header)) {
    case 1: // always be full-security
    case 2: // may negotiate downward
        // Control messages require full security.
        switch(vss_get_command(header) & 0x7f) {
        case VSS_NEGOTIATE:
        case VSS_TUNNEL_RESET:
        case VSS_AUTHENTICATE:
        case VSS_PROXY_REQUEST:
        case VSS_PROXY_CONNECT:
            break;
        default:
            if(signing_mode == VSS_NEGOTIATE_SIGNING_MODE_NONE ||
               sign_type == VSS_NEGOTIATE_SIGN_TYPE_NONE) {
                goto signature_ok;
            }
            break;
        }
        
        compute_hmac((char*)header, VSS_HEADER_SIZE,
                     subSession->signing_key,
                     calc_hmac, VSS_HMAC_SIZE);
        break;
    default:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Header version %d not known.", 
                         vss_get_version(header));
        return VSSI_BADVER;
    }

    if(memcmp(orig_hmac, calc_hmac, VSS_HMAC_SIZE) != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Header HMAC mismatch! security:%d.%d",
                         signing_mode, sign_type);
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Recived:");
        VPLTRACE_DUMP_BUF_FINE(TRACE_BVS, 0,
                               orig_hmac, VSS_HMAC_SIZE);
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Expected:");
        VPLTRACE_DUMP_BUF_FINE(TRACE_BVS, 0,
                               calc_hmac, VSS_HMAC_SIZE);
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Handle: %016"PRIx64".",
                          vss_get_handle(header));
        rv = VSSI_BADSIG;
        goto exit;
    }
 signature_ok:

    VPLMutex_Lock(&mutex);

    switch(vss_get_version(header)) {
    case 1:
        // If the XID is 0 and the command is an OPEN, reset sequence state.
        // This allows a session to be reused indefinitely, since the first
        // command to VSCS on restart of a client must be an OPEN command.
        if(vss_get_xid(header) == 0 &&
           (vss_get_command(header) == VSS_OPEN ||
            vss_get_command(header) == VSS_DELETE)) {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Resetting sequence state on XID 0 used for OPEN command.");
            subSession->sequence_state.reset();
        }
        break;
    case 2:
        // May reset XID state on negotiation success, but not here.
        break;
    }

    // Verify sequence number, unless this is a control message.
    if((vss_get_command(header) & 0x70) == 0x70) {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Skip XID check for proxy connection request.");
    }
    else if(subSession->sequence_state.is_valid(vss_get_xid(header))) {
        subSession->sequence_state.accept(vss_get_xid(header));
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to validate XID %u for command %x",
                         vss_get_xid(header), vss_get_command(header));
        rv = VSSI_BADXID;
    }
   
    VPLMutex_Unlock(&mutex);

 exit:
    return rv;
}

int vss_session::verify_header(const char* req, bool& verify_device)
{
    vss_sub_session* subSession = get_sub_session(vss_get_device_id(req));

    if ( subSession == NULL ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "No subsession for request - device "
            FMTu64,vss_get_device_id(req));
        return VSSI_BADSIG;
    }

    return verify_header(req, verify_device,
                         subSession->signing_mode,
                         subSession->sign_type);
}

int vss_session::verify_header(const char* req,
                               u8 signing_mode,
                               u8 sign_type)
{
    bool verify_device;

    return verify_header(req, verify_device,
                         signing_mode, sign_type);
}

int vss_session::verify_header(const char* header)
{
    bool verify_device;

    return verify_header(header, verify_device);
}
 
int vss_session::validate_request_data(char* req,
                                       u8 signing_mode,
                                       u8 sign_type,
                                       u8 encrypt_type)
{
    int rv = VSSI_SUCCESS;

    size_t data_length = vss_get_data_length(req);
    vss_sub_session* subSession = get_sub_session(vss_get_device_id(req));

    if ( subSession == NULL ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Missing dev spec key.");
        return VSSI_BADSIG;
    }

    // Verify signature.
    if(data_length > 0) { // 0-length data always OK. Nothing to verify.
        char calc_hmac[VSS_HMAC_SIZE];
        bool decrypt = true;

        switch(vss_get_version(req)) {
        case 1: // always be full-security
        case 2: // may negotiate downward
            // Control messages are always encrypted.
            switch(vss_get_command(req) & 0x7f) {
            case VSS_NEGOTIATE:
            case VSS_TUNNEL_RESET:
            case VSS_AUTHENTICATE:
            case VSS_PROXY_REQUEST:
            case VSS_PROXY_CONNECT:
                break;
            default:
                if(encrypt_type != VSS_NEGOTIATE_ENCRYPT_TYPE_AES128) {
                    decrypt = false;
                }
                break;
            }

            if(decrypt) {
                // Decrypt the data first. Use temp buffer.
                char* tmp = (char*)malloc(data_length);
                if(tmp == NULL) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Failed to alloc temp buffer to decrypt reply.");
                    return VSSI_NOMEM;
                }
                decrypt_data(tmp, req + VSS_HEADER_SIZE, data_length, 
                             req, subSession->encryption_key);
                // Swap buffers.
                memcpy(req + VSS_HEADER_SIZE, tmp, data_length);
                free(tmp);
            }
            
            // Control messages are always signed.
            switch(vss_get_command(req) & 0x7f) {
            case VSS_NEGOTIATE:
            case VSS_TUNNEL_RESET:
            case VSS_AUTHENTICATE:
            case VSS_PROXY_REQUEST:
            case VSS_PROXY_CONNECT:
                break;
            default:
                if(signing_mode != VSS_NEGOTIATE_SIGNING_MODE_FULL ||
                   sign_type == VSS_NEGOTIATE_SIGN_TYPE_NONE) {
                    goto signature_ok;
                }
                break;
            }

            compute_hmac(req + VSS_HEADER_SIZE,
                         data_length,
                         subSession->signing_key,
                         calc_hmac, VSS_HMAC_SIZE);
            break;
        default:
            // Header verification should deal with invalid versions.
            break;
        }

        if(memcmp(calc_hmac, vss_get_data_hmac(req), VSS_HMAC_SIZE) != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Data HMAC mismatch! security: %d.%d.%d",
                             signing_mode, sign_type, encrypt_type);
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Recived:");
            VPLTRACE_DUMP_BUF_FINE(TRACE_BVS, 0,
                                   vss_get_data_hmac(req), VSS_HMAC_SIZE);
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Expected:");
            VPLTRACE_DUMP_BUF_FINE(TRACE_BVS, 0,
                                   calc_hmac, VSS_HMAC_SIZE);
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Handle: %016"PRIx64".",
                              vss_get_handle(req));
            return VSSI_BADSIG;
        }
    }
    signature_ok:

    return rv;
}

int vss_session::validate_request_data(char* req)
{
    vss_sub_session* subSession = get_sub_session(vss_get_device_id(req));

    if ( subSession == NULL ) {
        return VSSI_BADSIG;
    }

    return validate_request_data(req,
                                 subSession->signing_mode,
                                 subSession->sign_type,
                                 subSession->encrypt_type);
}

int vss_session::validate_request_data(char* header, char* data,
                                       u8 signing_mode,
                                       u8 sign_type,
                                       u8 encrypt_type)
{
    int rv = VSSI_SUCCESS;

    size_t data_length = vss_get_data_length(header);
    vss_sub_session* subSession = get_sub_session(vss_get_device_id(header));

    if ( subSession == NULL ) {
        return VSSI_BADSIG;
    }

    // Verify signature.
    if(data_length > 0) { // 0-length data always OK. Nothing to verify.
        char calc_hmac[VSS_HMAC_SIZE];
        bool decrypt = true;

        switch(vss_get_version(header)) {
        case 1: // always be full-security
        case 2: // may negotiate downward
            // Decrypt the data first. Use temp buffer.
            // Control messages are always encrypted.
            switch(vss_get_command(header) & 0x7f) {
            case VSS_NEGOTIATE:
            case VSS_TUNNEL_RESET:
            case VSS_AUTHENTICATE:
            case VSS_PROXY_REQUEST:
            case VSS_PROXY_CONNECT:
                break;
            default:
                if(encrypt_type != VSS_NEGOTIATE_ENCRYPT_TYPE_AES128) {
                    decrypt = false;
                }
                break;
            }

            if(decrypt) {
                char* tmp = (char*)malloc(data_length);
                if(tmp == NULL) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Failed to alloc temp buffer to decrypt reply.");
                    return VSSI_NOMEM;
                }
                
                decrypt_data(tmp, data, data_length, 
                             header, subSession->encryption_key);
                // Swap buffers.
                memcpy(data, tmp, data_length);
                free(tmp);
            }

            // Control messages are always signed.
            switch(vss_get_command(header) & 0x7f) {
            case VSS_NEGOTIATE:
            case VSS_TUNNEL_RESET:
            case VSS_AUTHENTICATE:
            case VSS_PROXY_REQUEST:
            case VSS_PROXY_CONNECT:
                break;
            default:
                if(signing_mode != VSS_NEGOTIATE_SIGNING_MODE_FULL ||
                   sign_type == VSS_NEGOTIATE_SIGN_TYPE_NONE) {
                    goto signature_ok;
                }
                break;
            }

            compute_hmac(data, data_length,
                         subSession->signing_key,
                         calc_hmac, VSS_HMAC_SIZE);
            break;
        default:
            // Header verification should deal with invalid versions.
            break;
        }

        if(memcmp(calc_hmac, vss_get_data_hmac(header), VSS_HMAC_SIZE) != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Data HMAC mismatch! security: %d.%d.%d",
                             signing_mode, sign_type, encrypt_type);
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Header:");
            VPLTRACE_DUMP_BUF_FINEST(TRACE_BVS, 0,
                                     header, VSS_HEADER_SIZE);
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Computed:");
            VPLTRACE_DUMP_BUF_FINEST(TRACE_BVS, 0,
                                     calc_hmac, VSS_HMAC_SIZE);
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Data (%d bytes):", data_length);
            VPLTRACE_DUMP_BUF_FINEST(TRACE_BVS, 0,
                                     data, data_length);
            return VSSI_BADSIG;
        }
    }
    signature_ok:

    return rv;
}

int vss_session::validate_request_data(char* header, char* data)
{
    vss_sub_session* subSession = get_sub_session(vss_get_device_id(header));

    if ( subSession == NULL ) {
        return VSSI_BADSIG;
    }

    return validate_request_data(header, data,
                                 subSession->signing_mode,
                                 subSession->sign_type,
                                 subSession->encrypt_type);
}

void vss_session::sign_reply(char*& reply,
                             u8 signing_mode,
                             u8 sign_type,
                             u8 encrypt_type)
{
    // Compute signature for data and header.
    size_t data_len = vss_get_data_length(reply);
    vss_sub_session* subSession = get_sub_session(vss_get_device_id(reply));
    bool encrypt = true;
    bool sign = true;
    
    if ( subSession == NULL ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Missing dev spec key");
        return;
    }

    // Set the session handle used.
    vss_set_handle(reply, session_handle);

    if(data_len > 0) {

        switch(vss_get_version(reply)) {
        case 1: // always be full-security
        case 2: // may negotiate downward
            // Control responses always encrypt.
            switch(vss_get_command(reply) & 0x7f) {
            case VSS_NEGOTIATE:
            case VSS_TUNNEL_RESET:
            case VSS_AUTHENTICATE:
            case VSS_PROXY_REQUEST:
            case VSS_PROXY_CONNECT:
                break;
            default:
                if(encrypt_type != VSS_NEGOTIATE_ENCRYPT_TYPE_AES128) {
                    encrypt = false;
                }
                break;
            }
            if(encrypt) {
                // Pad data.
                size_t pad_len = ((CSL_AES_BLOCKSIZE_BYTES - 
                                   (data_len % CSL_AES_BLOCKSIZE_BYTES)) %
                                  CSL_AES_BLOCKSIZE_BYTES);
                data_len += pad_len;
                // Extend buffer for pad.
                char* tmp = (char*)realloc(reply, VSS_HEADER_SIZE + data_len);
                if(tmp == NULL) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Failed to allocate padded buffer for reply.");
                    return;
                }
                reply = tmp;
                vss_set_data_length(reply, data_len);
                
                // Add random pad bytes.
                while(pad_len > 0) {
                    // Random number generator good for 16 bits of randomness.
                    u16 random = get_randomness();
                    reply[VSS_HEADER_SIZE + data_len - pad_len] = random & 0xff;
                    pad_len--;
                    if(pad_len > 0) {
                        reply[VSS_HEADER_SIZE + data_len - pad_len] = (random >> 8) & 0xff;
                        pad_len--;
                    }
                }
            }
        
            // Control responses are always signed.
            switch(vss_get_command(reply) & 0x7f) {
            case VSS_NEGOTIATE:
            case VSS_TUNNEL_RESET:
            case VSS_AUTHENTICATE:
            case VSS_PROXY_REQUEST:
            case VSS_PROXY_CONNECT:
                break;
            default:
                if(signing_mode != VSS_NEGOTIATE_SIGNING_MODE_FULL ||
                   sign_type != VSS_NEGOTIATE_SIGN_TYPE_SHA1) {
                    sign = false;
                }
                break;
            }
            
            if(sign) {
                // Sign data
                compute_hmac(reply + VSS_HEADER_SIZE, data_len,
                             subSession->signing_key,
                             vss_get_data_hmac(reply), VSS_HMAC_SIZE);
            }
            else {
                memset(vss_get_data_hmac(reply), 0, VSS_HMAC_SIZE);
            }

            if(encrypt) {
                // Encrypt data. Use temp buffer.
                char* tmp = (char*)malloc(data_len);
                if(tmp == NULL) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Failed to allocate buffer to encrypt reply.");
                    return;
                }
                
                encrypt_data(tmp, reply + VSS_HEADER_SIZE, data_len, 
                             reply, subSession->encryption_key);
                memcpy(reply + VSS_HEADER_SIZE, tmp, data_len);
                free(tmp);
            }
            break;
        default:
            break;
        }
    }
    else {
        memset(vss_get_data_hmac(reply), 0, VSS_HMAC_SIZE);
    }

    memset(vss_get_header_hmac(reply), 0, VSS_HMAC_SIZE);
    switch(vss_get_version(reply)) {
    case 1: // always be full-security
    case 2: // may negotiate downward
        sign = true;
        // Control responses are always signed.
        switch(vss_get_command(reply) & 0x7f) {
        case VSS_NEGOTIATE:
        case VSS_TUNNEL_RESET:
        case VSS_AUTHENTICATE:
        case VSS_PROXY_REQUEST:
        case VSS_PROXY_CONNECT:
            break;
        default:
            if(signing_mode == VSS_NEGOTIATE_SIGNING_MODE_NONE ||
               sign_type != VSS_NEGOTIATE_SIGN_TYPE_SHA1) {
                sign = false;
            }
            break;
        }
        
        if(sign) {
            compute_hmac(reply, VSS_HEADER_SIZE,
                         subSession->signing_key,
                         vss_get_header_hmac(reply), VSS_HMAC_SIZE);
        }
        else {
            memset(vss_get_header_hmac(reply), 0, VSS_HMAC_SIZE);
        }
        break;
    default:
        break;
    }
}

void vss_session::sign_reply(char*& reply)
{
    vss_sub_session* subSession = get_sub_session(vss_get_device_id(reply));

    if ( subSession == NULL ) {
        return;
    }

    sign_reply(reply, subSession->signing_mode,
               subSession->sign_type, subSession->encrypt_type);
}

vss_object* vss_session::new_object(dataset* dataset,
                                    u64 origin_device,
                                    u64 user_id,
                                    u32 flags)
{
    vss_object* rv;

    VPLMutex_Lock(&mutex);

    // 0 is not a valid object handle
    // Safeguard in the case that the object handle wraps around
    if (object_handle_hint == 0) {
        object_handle_hint = 1;
    }

    while(objects.find(object_handle_hint) != objects.end()) {
        object_handle_hint++;
        if (object_handle_hint == 0) {
            object_handle_hint = 1;
        }
    }

    rv = objects[object_handle_hint] = new vss_object(dataset, origin_device, 
                                                      user_id, flags, 
                                                      object_handle_hint,
                                                      session_handle);

    object_handle_hint++;

    VPLMutex_Unlock(&mutex);

    return rv;
}

vss_object* vss_session::find_object(u32 handle, bool doAccess)
{
    vss_object* rv = NULL;

    VPLMutex_Lock(&mutex);

    map<u32, vss_object*>::iterator it = objects.find(handle);
    if(it == objects.end()) {
        rv = NULL;
    }
    else {
        rv = it->second;
        rv->reserve(doAccess);
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Object %p (handle:%d) refcount:%u",
                          rv, handle, rv->get_refcount());
    }

    VPLMutex_Unlock(&mutex);

    return rv;
}

void vss_session::find_object_handles(u64 uid, u64 did, std::vector<u32>& handle_list)
{
    vss_object* rv = NULL;

    VPLMutex_Lock(&mutex);

    map<u32, vss_object*>::iterator it;
    for(it = objects.begin();
        it != objects.end();
        it++) {
        if(it->second->get_dataset()->get_id().uid == uid &&
           it->second->get_dataset()->get_id().did == did) {
            rv = it->second;
            handle_list.push_back(it->first);
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Object %p (handle:%d)",
                              rv, rv->get_handle());
        }
    }
    VPLMutex_Unlock(&mutex);
}

bool vss_session::try_clear_timeout_objects()
{
    //clear object is low priority
    if(VPLMutex_TryLock(&mutex) == VPL_OK) {
        vss_object* object = NULL;
        map<u32, vss_object*>::iterator it, tmp;
        VPLTime_t cur_time = VPLTime_GetTimeStamp();
        for(tmp = objects.begin(); tmp != objects.end(); ) {
            it=tmp++;
            object = it->second;
            if(object != NULL) {
                if(cur_time > object->get_last_client_access_time() +
                    VPLTIME_FROM_SEC(vss_server::OBJECT_INACTIVE_TIMEOUT_SEC)) {
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "Timeout for object "FMTu32": "FMT_VPLTime_t"us have passed since last access.",
                                      object->get_handle(),
                                      VPLTime_DiffClamp(cur_time, object->get_last_client_access_time()));
                    if(!object->is_closed())
                        object->close();
                    if(it->second->get_refcount() == 0) {
                        delete it->second;
                        objects.erase(it);
                    }
                }
            }
        }
        VPLMutex_Unlock(&mutex);
    }
    else {
        return false;
    }
    return true;
}

void vss_session::release_object(u32 handle)
{
    vss_object* object = NULL;
    VPLMutex_Lock(&mutex);

    release_object_unlocked(handle, object);

    VPLMutex_Unlock(&mutex);

    if(object != NULL) {
        delete object;
    }
}

void vss_session::release_object_unlocked(u32 handle, vss_object*& object)
{
    object = NULL;
    map<u32, vss_object*>::iterator it = objects.find(handle);
    if(it != objects.end()) {
        it->second->release();
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Object %p (handle:%d) refcount:%u",
                          it->second, handle, it->second->get_refcount());
        if(it->second->get_refcount() == 0) {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Destroying object with handle %d.",
                              handle);
            object = it->second;
            objects.erase(it);
        }
    }
}

vss_sub_session::vss_sub_session(u64 device_id, 
                                 u64 session_handle, 
                                 const std::string& dsServiceTicket) :
    device_id(device_id),
    // Start at maximum security settings. Must negotiate down.
    signing_mode(VSS_NEGOTIATE_SIGNING_MODE_FULL),
    sign_type(VSS_NEGOTIATE_SIGN_TYPE_SHA1),
    encrypt_type(VSS_NEGOTIATE_ENCRYPT_TYPE_AES128)
{
    string text;

    // We only support device specific tickets
    // This must be a device specific ticker or no key.
    {
        stringstream stream;
        stream << "Encryption Key " 
               << hex << uppercase <<setfill('0') << setw(16) << device_id << " "
               << hex << uppercase <<setfill('0') << setw(16) << session_handle;
        text = stream.str();
    }
    compute_hmac((char*)(text.data()), text.size(),
                 dsServiceTicket.data(),
                 encryption_key, CSL_AES_KEYSIZE_BYTES);
    
    {
        stringstream stream;
        stream << "Signing Key " 
               << hex << uppercase <<setfill('0') << setw(16) << device_id << " "
               << hex << uppercase <<setfill('0') << setw(16) << session_handle;
        text = stream.str();
    }
    compute_hmac((char*)(text.data()), text.size(),
                 dsServiceTicket.data(),
                 signing_key, CSL_SHA1_DIGESTSIZE);
} 

vss_sub_session* vss_session::get_sub_session(u64 device_id)
{
    map<u64, vss_sub_session*>::iterator it;
    vss_sub_session* ret_ses = NULL;
    
    VPLMutex_Lock(&mutex);

    it = sub_sessions.find(device_id);
    if(it == sub_sessions.end()) {
        u64 effective_device_id = device_id & 0xffffffffffull;
        std::string dsServiceTicket;
        (void)query->getDeviceSpecificTicket(uid, effective_device_id, dsServiceTicket);
        if ( dsServiceTicket.size() != CSL_SHA1_DIGESTSIZE ) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "No device spec ticket found for "
                "User: "FMTu64" dev id: "FMTu64, uid, effective_device_id);
            goto exit;
        }
        sub_sessions[device_id] = new vss_sub_session(device_id,
                                                      session_handle, 
                                                      dsServiceTicket);
        it = sub_sessions.find(device_id);
    }
    ret_ses = it->second;
 exit:
    VPLMutex_Unlock(&mutex);
    
    return ret_ses;
}
