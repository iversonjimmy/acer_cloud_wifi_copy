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

#ifndef __HTTP_RESPONSE_HPP__
#define __HTTP_RESPONSE_HPP__

#include "vplu_types.h"
#include "vplu_common.h"
#include "vplex_util.hpp"

#include <string>
#include <map>

class http_response {
private:
    VPL_DISABLE_COPY_AND_ASSIGN(http_response);

public:
    int response;                               /// Response code
    typedef std::map<std::string, std::string, case_insensitive_less> header_list;
    header_list headers;                        /// Response headers
    std::string content;                        /// Response content buffer

    bool response_done;  // true iff http status received/set
    bool headers_done;   // true iff all headers received/set
    u64 body_len;        // size of body; conditioned no headers_done
    u64 body_so_far;     // body received so far
    std::string line_so_far;  // internal work buffer??

    // public methods
    http_response();
    ~http_response();

    void add_response(int response);
    void add_response(std::string line); // deliberately passing by value
    void add_header(const std::string& header);
    void add_header(const std::string& hname, const std::string& val);
    const std::string* find_header(const std::string &key) const;
    void add_content(const char* data, size_t len);
    void add_content(const std::string& content);

    // Accept input from a block of data.
    // Return amount of data consumed.
    size_t add_input(const char* inbuf, size_t length);

    /// Dump the response headers to a string.
    void dump_headers(std::string& out);
    /// Dump the response information to a string.
    void dump(std::string& out);
    /// Dump the response information to stdout, for debug.
    void dump();
    
    void reset_state();

    /// There is new data (header or content) available to be sent.
    bool has_data() const;
    /// Extract data of response for sending.
    /// Bytes returned are removed from the response data.
    /// Returns 0 when no further bytes remain.
    size_t get_data(char* buf, size_t len);

    /// Indicate if all data for this response has been fetched via get_data().
    bool send_done() const;
    bool send_header_done() const;

    /// If reply data is long, set this callback instead of adding the content.
    /// Will be called until the callback indicates EOF reached.
    void set_data_fetch_callback(size_t (fetch_fn)(char*, size_t, void*, bool&),
                                 void* ctx);

    /// For HEAD requests, call to block accumulation of response body.
    void set_no_body();

private:
    std::string send_buf; // Temporary buffer for header, short content data to send.
    bool sent_header;     // True when header has been put into send_buf.
    bool sent_body;       // True when entire body content has been sent.
    bool no_body;         // True when no body data will exist (HEAD request)

    // Callback for fetching data.
    // Params: data buffer, buffer size, context, EOF flag.
    // Returns: Amount of data put into buffer.
    size_t (*data_fetch)(char*, size_t, void*, bool&);
    // Context for above fetch call.
    void* data_fetch_ctx;
};

#endif //include guard
