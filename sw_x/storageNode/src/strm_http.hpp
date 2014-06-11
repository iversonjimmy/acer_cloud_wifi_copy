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

/// streaming server http client class
/// A single client HTTP connection as served by streaming server. this class builds on the
/// generic_http class

#ifndef __STRM_HTTP_HPP__
#define __STRM_HTTP_HPP__

#include "vplu_common.h"
#include "vpl_time.h"
#include "vpl_socket.h"
#include "vplex_file.h"
#include "vplex_util.hpp"

#include "vssi_types.h"
#include "vss_comm.h"
#include "vss_file.hpp"

// forward declaration
class strm_http;

#include <queue>
#include <map>

#include "http_request.hpp"
#include "http_response.hpp"
#include "vss_server.hpp"
#include "vss_session.hpp"

#define IGWARE_SERVER_STRING                    "Acer Media Streaming Server"

#include "sn_features.h"  // May define ENABLE_PHOTO_TRANSCODE
#ifdef ENABLE_PHOTO_TRANSCODE
#include "image_transcode.h"
#endif

enum {
    MEDIA_TYPE_UNKNOWN = 0,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_PHOTO
};

struct RangeHeader {
    VPLFile_offset_t start;
    VPLFile_offset_t end;
    std::string header;

    RangeHeader()
    : start(0), end(0)
    {}
};

class strm_http_transaction {
public:
    // states of a transaction
    bool receiving; // True when receiving any data. False when receive done.
    bool processing; // True when processing request, even if still receiving.
    bool sending; // True when response to send has been set.
    bool failed;

    http_request req;
    http_response resp;

    // Timestamp of transaction creation.
    VPLTime_t start;

    // Bytes received/sent for transaction
    u64 recv_cnt;
    u64 send_cnt;

    strm_http* http_handle;

    // Fields used to send content.
    // There are two cases to consider, depending on whether the file is in a dataset or not.
    // (1) If the file is in a dataset, info needed are dataset, component, current offset, and end offset.
    // (2) If the file is outside a dataset, info needed are file path, file handle, current offset, and end offset.
    std::string         content_file;  // (1) component; (2) file path
    vss_file*           vss_file_handle;  // (1) component

#ifdef ENABLE_PHOTO_TRANSCODE
    bool                is_image_transcoding;
    ImageTranscode_handle_t image_transcoding_handle;
#endif

    VPLFile_handle_t    fh;            // (1) not used, should be VPLFILE_INVALID_HANDLE; (2) file handle
    dataset*            ds;            // (1) dataset; (2) not used, should be NULL
    VPLFile_offset_t    cur_offset;    // (1,2) current offset
    VPLFile_offset_t    end_offset;    // (1,2) end offset

    bool lock_is_wrlock;

    // multi-range fields used to send content
    std::vector<RangeHeader> ranges;  // (start, end)
    u32 rangeIndex;                            // points to the index within range
    bool rangeIndexHeaderSent;    // indicates whether the header at rangeIndex
                                  // has beens sent or not.
    std::string footer;           // footer
    bool write_file;

    strm_http_transaction(strm_http* handle);
    ~strm_http_transaction();
};

class strm_http
{
public:
    // Communication socket and state for the client
    VPLSocket_t sockfd;
    bool receiving;
    bool sending;
    bool disconnected;
    VPLTime_t last_active;
    VPLTime_t inactive_timeout;
    int conn_type;

    strm_http(vss_server& server, VPLSocket_t sockfd, int conn_type,
              vss_session* session = NULL,
              u64 device_id = 0,
              bool need_auth = false);
    ~strm_http();
    
    // Add a method handler. Only the last method handler for a given method
    // will be executed when a request for that method arrives.
    // Handler's args: pointer to http session, 
    //                 pointer to transaction
    void register_method_handler(std::string& method,
                                 void (*handler)(strm_http*,
                                                 strm_http_transaction*));

    // Start a http request handler. Use when client first connects.
    int start(VPLTime_t inactive_timeout);

    void do_send();

    void do_receive();

    // Put a response for eventual sending to client.
    void put_response(strm_http_transaction* transaction);
    // Put a proxy connect response to the client.
    void put_proxy_response(const char* msg, size_t length);

    // Clients have three states: 
    // * active (working on a request)
    // * idle (connected, but no request receiving or in progress)
    // * inactive (disconnected and delete-able)
    // Is client actively processing a request?
    bool active();
    // Is client inactive (disconnected or timed-out)?
    bool inactive();

    // Disconnect the http request handler.  Will cause to be cleaned up on next
    // timeout.
    void disconnect();

    std::map<std::string, std::string, case_insensitive_less> video_mime_map;
    std::map<std::string, std::string, case_insensitive_less> audio_mime_map;
    std::map<std::string, std::string, case_insensitive_less> photo_mime_map;

    // return the storage node device id
    u64 get_device_id();

    vss_server& server;
private:
    VPL_DISABLE_COPY_AND_ASSIGN(strm_http);

    vss_session* session;
    bool need_auth;

    u64 my_device_id;
    u64 device_id;
    u32 next_xid;

    // Map of method handlers. 
    // HEAD is automatically handled by the GET handler.
    std::map<std::string, void (*)(strm_http*, strm_http_transaction*)> handlers;
 
    // Queues of requests and responses pending
    std::queue<strm_http_transaction*> transaction_queue;
    std::queue<std::pair<size_t, const char*> > send_queue;
    size_t sent_so_far; // for head reply in-progress
    static const size_t SEND_BUFSIZE = (32 * 1024);
    static const size_t RECV_BUFSIZE = (8 * 1024);
    void get_chunk_to_send();

    // Purge non-completable transactions
    void purge_transactions();

    // Begin processing a single transaction.
    void handle_transaction(strm_http_transaction* transaction);

    // Security level information
    u8 signing_mode;
    u8 sign_type;
    u8 encrypt_type;
    u8 proto_version;

    // Receive state
    char incoming_hdr[VSS_HEADER_SIZE]; // with secure connection
    size_t reqlen; // with secure connection
    size_t req_so_far; // with secure connection
    char* incoming_body; 
    bool recv_error;
    size_t receive_buffer(char** buf_out);
    void handle_received(int bufsize);
    void digest_input(const char* data, size_t length);

    // When auth needed, authenticate based on first packet received.
    void authenticate_connection();

    // When reset request received, perform reset.
    void reset_stream();

    void transaction_cleanup(strm_http_transaction* transaction, bool aborted);
    void log_completion(strm_http_transaction* transaction, bool aborted=false);

    //std::string remotefile temporarily folder path
    std::string remotefile_tmp_folder;

    s16 write_upload_file_req(strm_http_transaction* transaction);
};

// Get the contents of a file or directory.
void handle_get_or_head(strm_http* http, strm_http_transaction* transaction);
void handle_put_post_n_del(strm_http* http, strm_http_transaction* transaction);

#endif // include guard
