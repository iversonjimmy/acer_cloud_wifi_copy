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

#ifndef __HTTP_REQUEST_ASYNC_H__
#define __HTTP_REQUEST_ASYNC_H__

#include "http_request.hpp"
#include "vplex_file.h"

class http_request_async :
    public http_request
{
private:
    std::string line;        /// partial input line of a request
    static const int GET_REQLINE  = 0;
    static const int GET_HEADERS  = 1;
    static const int GET_CONTENT  = 2;
    static const int GET_COMPLETE = 3;
    int recv_state;

    const std::string* filepath;
    VPLFile_offset_t cur_offset;
    VPLFile_handle_t upload_file;
    /// Read data from a local file for async upload
    ssize_t read_content_from_file(const char* file, char *buf, size_t len, u64 offset);

public:
    http_request_async(void);
    virtual ~http_request_async(void);

    // Read bytes of request content received so far.
    // Starting content byte index returned.
    u64 read_content(std::string& buf);
    /// Add the received data to this request
    /// Return number of bytes consumed.
    size_t receive(const char* buf, size_t len);

};

#endif //include guard
