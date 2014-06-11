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

#include "http_request_async.hpp"
#include "vplex_trace.h"

#include <string>
#include <sstream>

using namespace std;

http_request_async::http_request_async(void)
    : recv_state(GET_REQLINE),
    filepath(NULL),
    cur_offset(0),
    upload_file(VPLFILE_INVALID_HANDLE)
{
}

http_request_async::~http_request_async(void)
{
    if(VPLFile_IsValidHandle(upload_file)) {
        VPLFile_Close(upload_file);
    }
}

u64 http_request_async::read_content(std::string& buf)
{
    u64 rv = content_read;
    ssize_t read_len = 0;
#if defined(WIN32) || defined(CLOUDNODE) || defined(LINUX)
    ssize_t length = (1024 * 1024);
#else
    ssize_t length = (64 * 1024);
#endif
    char *readbuf = new char[length];

    if(filepath != NULL) {
        read_len = read_content_from_file(filepath->c_str(), readbuf, length, cur_offset);
        if(read_len > 0) {
            buf.append(readbuf, read_len);
            content_read += read_len;
        }
    }
    delete readbuf;
    return rv;
}

ssize_t http_request_async::read_content_from_file(const char* file, char *buf, size_t len, u64 offset)
{
    ssize_t rv = 0;
    u64 content_len = 0;

    content_len = content_length();
    if (upload_file == VPLFILE_INVALID_HANDLE) {
        upload_file = VPLFile_Open(file, VPLFILE_OPENFLAG_READONLY, 0);
        if(!VPLFile_IsValidHandle(upload_file)) {
            rv = -1;
        }
    }

    if (upload_file != VPLFILE_INVALID_HANDLE) {
        if (len > static_cast<size_t>(content_len - offset)) {
            // Fetch no further than the last byte of request range.
            len = static_cast<size_t>(content_len - offset);
        }
        if (offset == content_len) {
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,"Read file completely.");
            return rv;
        }
        else {
            // Get body data from file via set callback and context.
            rv = VPLFile_ReadAt(upload_file, buf, len, offset);
            if (rv < 0) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Error while reading file %s at offset "FMTu64", err = "FMT_ssize_t,
                                 file, offset, rv);
                VPLFile_Close(upload_file);
                upload_file = VPLFILE_INVALID_HANDLE;
                return rv;
            }
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "Fetched "FMT_ssize_t" bytes.", rv);
            cur_offset += rv;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Write from file "FMT_ssize_t" bytes. offset: "FMTu64"; cur_offset:"FMTu_VPLFile_offset_t, rv, offset, cur_offset);
        }
        if(content_len == cur_offset) {
            VPLFile_Close(upload_file);
            upload_file = VPLFILE_INVALID_HANDLE;
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Pulled "FMTu64" bytes. Done.",
                        content_len);
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                "Pulled "FMTu64" bytes. Done.", content_len);
        }
    }

    return rv;
}

static void strip_newline(string& line)
{
    size_t pos = line.length() - 1;

    if (line.length() != 0) {
        while(line[pos] == '\n' || line[pos] == '\r') {
            line.erase(pos, 1);
            pos--;
            if (line.length() == 0) 
                break;
        }
    }
}

size_t http_request_async::receive(const char* buf, size_t len)
{
    size_t rv = 0;
    size_t beginline = 0;

    if((recv_state == GET_REQLINE) ||
       (recv_state == GET_HEADERS)) {
        while(beginline < len) {
            // Break input down into lines. Keep any partial line.
            // Lines end in "\n", with possible "\r\n" endings.
            // Lines may be empty.
            
            size_t linelen = 0;
            for(size_t i = beginline; i < len; i++) {
                linelen++;
                if(buf[i] == '\n') {
                    break;
                }
            }
            
            line.append(&(buf[beginline]), linelen);
            beginline += linelen;
            rv += linelen;
            if(*(line.end() - 1) != '\n') {
                // not enough for a complete line
                break;
            }
            
            if(recv_state == GET_REQLINE) {
                // Collect the command line
                strip_newline(line);
                if(line.length() > 0) {
                    add_info(line);
                    recv_state = GET_HEADERS;
                }
            }
            else {
                strip_newline(line);
                if(line.length() == 0) {
                    // Blank line after headers indicates end of headers.
                    if(http_method == "POST" ||
                       http_method == "PUT") {
                        const string* lenstr;
                        lenstr = find_header("Content-Length");
                        recv_state = GET_CONTENT;
                        if(lenstr != NULL) {            
                            content_len = strtoull(lenstr->c_str(), 0, 10);
                            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                                           "Expect content length "FMTu64,
                                           content_len);
                        }
                        else {
                            content_len = 0;
                        }
                    }
                    else {
                        recv_state = GET_COMPLETE;
                    }
                    break;
                }
                // Collect a header line
                add_header(line);
            }
            
            line.clear();
        }
    }

    // if we finished receiving header, then try to get the
    // file path and calculate the content-length
    if(recv_state == GET_CONTENT) {
        // Collect data into the content.
        if(beginline != len &&
           content_received < content_len) {
            size_t linelen = len - beginline;
            if(linelen > (static_cast<size_t>(content_len - content_received))) {
                linelen = static_cast<size_t>(content_len - content_received);
            }
            add_content(&(buf[beginline]), linelen);
            rv += linelen;
        }

        if(content_len == content_received) {
            recv_state = GET_COMPLETE;
        }

        // Find filepath from the http_request header where we
        // put it before we enqueue the async task
        if (http_method == "POST" &&
            (uri.find("/rf/file/") != std::string::npos ||
             uri.find("/media_rf/file/") != std::string::npos)) {
            filepath = find_header("x-ac-srcfile");
        }

        if(filepath == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,"empty file path");
            return -1;
        }

        s64 tmplSz;
        std::stringstream ss;
        if(upload_file == VPLFILE_INVALID_HANDLE) {
            upload_file = VPLFile_Open(filepath->c_str(), VPLFILE_OPENFLAG_READONLY, 0);
            if (!VPLFile_IsValidHandle(upload_file)) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,"invalid file handle of file : %s", filepath->c_str());
                return -1;
            }
        }
        tmplSz = (s64) VPLFile_Seek(upload_file, 0, VPLFILE_SEEK_END);
        if (tmplSz < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,"invalid file size of file : %s", filepath->c_str());
            return -1;
        }
        VPLFile_Seek(upload_file, 0, VPLFILE_SEEK_SET); // Reset
        ss << "Content-Length: ";
        ss << tmplSz;
        content_len = tmplSz;
        add_header(ss.str());
    }
    // file will be closed after http_request_async is destructed
    return rv;
}
