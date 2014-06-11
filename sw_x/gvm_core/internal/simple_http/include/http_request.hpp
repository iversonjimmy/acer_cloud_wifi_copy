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

#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include "vplu_types.h"
#include "vplu_common.h"
#include "vplex_file.h"
#include "vplex_util.hpp"

#include <ctype.h>
#include <string>
#include <map>

class http_request {
private:
    VPL_DISABLE_COPY_AND_ASSIGN(http_request);

    std::string line;        /// partial input line of a request
    //  See http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html
    static const int GET_REQLINE  = 0;
    static const int GET_HEADERS  = 1;
    static const int GET_CONTENT  = 2;
    static const int GET_COMPLETE = 3;
    int recv_state;

    std::string content;     // Request content (for PUT/POST)
    bool dump_content;       // If true, discard content as it is received.

    void parse_query_string();

protected:
    u64 content_len;      // # bytes of content expected
    u64 content_received; // # bytes received for the request
    u64 content_read;     // # bytes read for processing

public:
    std::string http_method;                    /// Request's HTTP method
    std::string uri;                            /// Request's URI
    std::string version;                        /// Request's HTTP version

    typedef VPLUtil_TimeOrderedMap<std::string, std::string, case_insensitive_less> header_list;
    header_list headers; /// Request headers

    typedef std::map<std::string, std::string> query_list;
    query_list query; /// fields in query string

    http_request();
    virtual ~http_request();
    
    void add_info(const std::string& request_line);
    void add_header(const std::string& header);
    const std::string* find_header(const std::string &key) const;
    void add_content(const char* data, size_t len);

    const std::string* find_query(const std::string &key) const;

    // Read bytes of request content received so far.
    // Starting content byte index returned.
    virtual u64 read_content(std::string& buf);
    // True when all request content has been read.
    bool read_done() const;
    // True when all expected bytes have been read (not truncated).
    bool read_complete() const;
    // Get current content available via read_content().
    size_t content_available() const;
    // Get total size of content.
    u64 content_length() const;
    // Dump all content received and which will be received.
    void purge_content();

    /// Dump the request information to a string.
    void dump(std::string& out);
    /// Dump the request information to stdout, for debug.
    void dump();
    /// Dump just the request header
    void dump_header(std::string& out);

    // Reset to original receive state
    void reset_state();
    
    /// Add the received data to this request
    /// Return number of bytes consumed.
    virtual size_t receive(const char* buf, size_t len);

    /// Indicate  if all headers have been received.
    bool headers_complete() const;
    /// Indicate if a whole request has been received.
    bool receive_complete() const;
};

#endif //include guard
