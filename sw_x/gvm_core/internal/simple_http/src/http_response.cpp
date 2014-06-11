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

#include <iostream> // for debug
#include <set>

#include <ctype.h>

#include "vplex_trace.h"

#include "http_response.hpp"

using namespace std;

http_response::http_response() :
    response(500),
    response_done(false),
    headers_done(false),
    body_len(0),
    body_so_far(0),
    sent_header(false),
    sent_body(false),
    no_body(false),
    data_fetch(NULL),
    data_fetch_ctx(NULL)
{ }

http_response::~http_response()
{ }

void http_response::add_response(int response)
{
    this->response = response;
}

void http_response::add_response(std::string line)
{
    // Find result code in the line
    size_t pos = line.find_first_of(' ');
    line.erase(0, pos); // if pos is npos, the entire line will be erased, and that's fine.
    sscanf(line.c_str(), "%d", &(this->response));    
}

void http_response::add_header(const std::string& hname, const std::string& val)
{
    // Combine headers with value lists.
    if(this->headers.find(hname) != this->headers.end()) {
        this->headers[hname] += ',' + val;
    }
    else {
        this->headers[hname] = val;
    }
}

void http_response::add_header(const std::string& header)
{
    size_t nameEnd = header.find_first_of(':');
    if (nameEnd == string::npos) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Invalid header format \"%s\"", header.c_str());
        return;
    }
    string name = header.substr(0, nameEnd);
    string value = header.substr(nameEnd + 1); // substr safely returns an empty string if nameEnd + 1 == header.size()

    // strip leading whitespace on value
    while(value.length() > 0 && isspace(value[0])) {
        value.erase(0, 1);
    }
    // strip trailing whitespace on value
    while(value.length() > 0 && isspace(value[value.length() - 1])) {
        value.erase(value.length() - 1, 1);
    }

    add_header(name, value);
}

const std::string* http_response::find_header(const std::string &key) const
{
    header_list::const_iterator it = this->headers.find(key);
    if(it != this->headers.end()) {
        return &(it->second);
    }
    else {
        return NULL;
    }
}

void http_response::add_content(const std::string& content)
{
    this->content.append(content);
    body_so_far += content.size();
}

void http_response::add_content(const char* data, size_t len)
{
    content.append(data, len);
    body_so_far += len;
}

size_t http_response::add_input(const char* inbuf, size_t length)
{
    size_t beginline = 0;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Accepting "FMTu_size_t" bytes of data from buffer %p.",
                        length, inbuf);

    while(length > beginline) {
        if(!response_done || !headers_done) {
            // Parse lines from buffer, breaking on \n characters.
            size_t linelen = 0;
            for(size_t i = beginline; i < length; i++) {
                linelen++;
                if(inbuf[i] == '\n') {
                    break;
                }
            }
            
            // Add line fragment to partial line. Append if needed.
            line_so_far.append(inbuf + beginline, linelen);
            beginline += linelen;
            
            // If line not complete, get more data.
            if((line_so_far.size() >= 1) && (line_so_far[line_so_far.size() - 1] != '\n')) {
                break;
            }

            // Strip '\r and '\n' from line
            while((line_so_far.size() >= 1) &&
                (line_so_far[line_so_far.size() - 1] == '\n' ||
                 line_so_far[line_so_far.size() - 1] == '\r')) {
                line_so_far.erase(line_so_far.size() - 1, 1);
            }

            // Collect response if needed
            if(!response_done) {
                VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                   "Response {%s} received.",
                                   line_so_far.c_str());
                add_response(line_so_far);
                response_done = true;
                line_so_far.clear();
            }
            else { // Collect headers
                // Check for end of headers
                if(line_so_far.empty()) {
                    // Blank line after headers indicates end of headers.
                    headers_done=true;
                    
                    if(no_body) {
                        body_len = 0;
                    }
                    else {
                        // Get body length from headers.
                        // TODO: Deal with chunked-encoding as well.
                        const string* length_hdr = find_header("Content-Length");
                        if(length_hdr) {
                            body_len = strtoull(length_hdr->c_str(), 0, 10);
                        }
                        else {
                            body_len = 0;
                        }
                    }

                    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                       "End of Headers received. "FMTu64" bytes of body expected.",
                                       body_len);
                    // Continue collecting body data.
                    continue;
                }
                else {
                    // Collect this header.
                    add_header(line_so_far);
                    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                       "Header {%s} received.",
                                       line_so_far.c_str());
                    line_so_far.clear();
                }
            }
        }
        else {
            size_t accept_len = length - beginline;
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                               "Receiving content body. Got "FMTu64"/"FMTu64" bytes so far. Have "FMTu_size_t" bytes available.",
                               body_so_far, body_len, accept_len);
            // Collect response body up to the response body expected.
            if(accept_len > body_len - body_so_far) {
                accept_len = (size_t)(body_len - body_so_far);
            }
            add_content(inbuf + beginline, accept_len);
            beginline += accept_len;
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                               "Receiving content body. Got "FMTu64"/"FMTu64" bytes so far. Accepted "FMTu_size_t" bytes.",
                                body_so_far, body_len, accept_len);

            if(body_len == body_so_far) {
                // Done for this request.
                break;
            }
        }
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "%p: Accepted "FMTu_size_t"/"FMTu_size_t" bytes of data from buffer %p.",
                        this, length, beginline, inbuf);

    return beginline;  
}

#define CRLF "\r\n"
static const string ok_200 = "HTTP/1.1 200 OK" CRLF;
static const string ok_206 = "HTTP/1.1 206 Partial Content" CRLF;
static const string ok_201 = "HTTP/1.1 201 Created" CRLF;
static const string ok_204 = "HTTP/1.1 204 No Content" CRLF;
static const string err_400 = "HTTP/1.1 400 Bad request" CRLF;
static const string err_401 = "HTTP/1.1 401 Unauthorized" CRLF;
static const string err_404 = "HTTP/1.1 404 File not found" CRLF;
static const string err_405 = "HTTP/1.1 405 Method Not Allowed" CRLF;
static const string err_409 = "HTTP/1.1 409 Conflict" CRLF;
static const string err_415 = "HTTP/1.1 415 Unsupported Media Type" CRLF;
static const string err_416 = "HTTP/1.1 416 Requested Range Not Satisfiable" CRLF;
static const string err_500 = "HTTP/1.1 500 Internal Server Error" CRLF;
static const string err_501 = "HTTP/1.1 501 Not Implemented" CRLF;
static const string err_503 = "HTTP/1.1 503 Service Unavailable" CRLF;
static const string err_504 = "HTTP/1.1 504 Gateway Timeout" CRLF;


void http_response::dump_headers(std::string& out)
{
    switch(response) {
    case 200: out = ok_200; break;
    case 201: out = ok_201; break;
    case 204: out = ok_204; break;
    case 206: out = ok_206; break;
    case 400: out = err_400; break;
    case 401: out = err_401; break;
    case 404: out = err_404; break;
    case 405: out = err_405; break;
    case 409: out = err_409; break;
    case 415: out = err_415; break;
    case 416: out = err_416; break;
    case 501: out = err_501; break;
    case 503: out = err_503; break;
    case 504: out = err_504; break;
    default: // When in doubt, it's a server error.
    case 500: out = err_500; break;
    }

    for(header_list::iterator it = headers.begin(); 
        it != headers.end();
        it++) {

        out += it->first + ": " + it->second + CRLF;
    }
    out += CRLF;
}

void http_response::dump(std::string& out)
{
    dump_headers(out);
    out += content;
}

void http_response::dump()
{
    string out;

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                   "Dumping Response:");

    dump_headers(out);
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                   "<begin headers>\n%s", out.c_str());
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                   "<end headers>");
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                   "%s", content.c_str());
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                   "End Response.");
}

void http_response::reset_state()
{
    response = 500;
    response_done = false;
    headers_done = false;
    body_len = 0;
    body_so_far = 0;
    sent_header = false;
    sent_body = false;
    data_fetch = NULL;
    data_fetch_ctx = NULL;

    headers.clear();
    content.clear();
    send_buf.clear();
    line_so_far.clear();
}

bool http_response::send_done() const
{
    if(sent_header && sent_body && send_buf.empty()) {
        return true;
    }
    return false;
}

bool http_response::send_header_done() const
{
    return sent_header;
}

/// There is new data (header or content) available to be sent.
bool http_response::has_data() const
{
    bool rv;
    rv = !sent_header ||
         !send_buf.empty() ||
         !content.empty();
    return rv;
}

size_t http_response::get_data(char* buf, size_t len)
{
    size_t rv = 0;

    if(!sent_header) {
        // Serialize header.
        dump_headers(send_buf);
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Headers:\n%s", send_buf.c_str());
        sent_header = true;
        
        // Determine body length if not known.
        if(body_len == 0 && !no_body) {
            const string* length_hdr = find_header("Content-Length");
            if(length_hdr) {
                body_len = strtoull(length_hdr->c_str(), 0, 10);
            }
        }

        // no need for this data anymore
        headers.clear();
    }

    if(!send_buf.empty()) {
        // Send available data.
        if(send_buf.size() < len) {
            rv = send_buf.size();
        }
        else {
            rv = len;
        }
        memcpy(buf, send_buf.data(), rv);
        send_buf.erase(0, rv);
    }
    else {
        if(data_fetch) {
            // Get body data from file via set callback and context.
            rv = data_fetch(buf, len, data_fetch_ctx, sent_body);
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Fetched "FMTu_size_t" bytes. Done:%d",
                                rv, sent_body);
        }
        else {
            // Get body data from response content.
            if(!content.empty()) {
                rv = content.size();
                if(rv > len) {
                    rv = len;
                }
                memcpy(buf, content.data(), rv);
                content.erase(0, rv);
            }
            if(no_body ||
               (content.empty() && body_so_far == body_len)) {
                sent_body = true;
            }
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Pulled "FMTu_size_t" bytes. Done:%d",
                                rv, sent_body);
        }
    }

    return rv;
}
void http_response::set_data_fetch_callback(size_t (fetch_fn)(char*, size_t, void*, bool&),
                                            void* ctx)
{
    data_fetch = fetch_fn;
    data_fetch_ctx = ctx;
}

void http_response::set_no_body()
{
    no_body = true;
}
