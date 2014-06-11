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

#include "http_request.hpp"

#include <set>

#include <ctype.h>
#include <locale>

#include "vplex_http_util.hpp"
#include "vplex_trace.h"

using namespace std;

http_request::http_request()
    : recv_state(GET_REQLINE), dump_content(false), content_len(0), content_received(0), content_read(0)
{}

http_request::~http_request()
{}

void http_request::reset_state()
{
    purge_content();
    recv_state = GET_REQLINE;
    content_len = 0; 
    content_received = 0; 
    content_read = 0; 
    dump_content = false;
    headers.clear();
    query.clear();
}

void http_request::parse_query_string()
{
    size_t query_pos = uri.find_first_of('?');
    if (query_pos == string::npos)
        return;

    // TODO: support '#' to indicate end of query component
    //       cf. http://tools.ietf.org/html/rfc3986#section-3.4
    size_t next = query_pos + 1;
    while (next != string::npos && next < uri.length()) {
        string field, value;
        size_t p = uri.find_first_of("=&;", next);
        if (p == string::npos) {  // last pair with empty value
            field.assign(uri, next, uri.length() - next);
            value.clear();
            next = p;
        }
        else if (uri[p] == '&' || uri[p] == ';') {  // pair with empty value
            field.assign(uri, next, p - next);
            value.clear();
            next = p + 1;
        }
        else {  // uri[p] == '='
            field.assign(uri, next, p - next);
            next = p + 1;
            if (next >= uri.length()) {
                value.clear();
            }
            else {
                p = uri.find_first_of("=&;", next);
                if (p == string::npos) {  // remainder is the value
                    value.assign(uri, next, uri.length() - next);
                    next = p;
                }
                else if (uri[p] == '=') {  // unexpected - try to recover by skipping past the next end-of-pair marker
                    next = p + 1;
                    if (next < uri.length()) {
                        next = uri.find_first_of("&;", next);
                    }
                    continue;
                }
                else {  // uri[p] == '&' || uri[p] == ';'
                    value.assign(uri, next, p - next);
                    next = p + 1;
                }
            }
        }
        if (!field.empty()) {
            std::string decoded_value;
            int err = VPLHttp_DecodeUri(value, decoded_value);
            if (!err) {
                query[field] = decoded_value;
            }
        }
    }
}

void http_request::add_info(const std::string& request_line)
{
    string substr;
    
    // Format is: method SP uri SP version
    
    // URI must be decoded for use.

    // Determine method requested
    size_t space = request_line.find_first_of(' ');
    substr = request_line.substr(0, space);
    http_method = substr;

    // Get URI - the part between spaces.
    uri = request_line.substr(space+1, 
                              request_line.find_first_of(' ', space+1)
                              - (space+1));

    parse_query_string();

    // Get version
    version = request_line.substr(request_line.find_last_of(' ')+1);
}

void http_request::add_header(const std::string& header)
{
    // Headers are of the form (name ':' field) with any amount of LWS
    // following the ':' before the field value. LWS after field is ignored.

    size_t nameEnd = header.find_first_of(':');
    if (nameEnd == string::npos) {
        // Bad header format!
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Invalid header format \"%s\"", header.c_str());
        return;
    }
    string name = header.substr(0, nameEnd);
    string value = header.substr(nameEnd + 1); // substr safely returns an empty string if nameEnd + 1 == header.size()

    // strip leading whitespace on value
    locale loc;
    while(value.length() > 0 && isspace(value[0], loc)) {
        value.erase(0, 1);
    }
    // strip trailing whitespace on value
    while(value.length() > 0 && isspace(value[value.length() - 1], loc)) {
        value.erase(value.length() - 1, 1);
    }

    // Possibly consolidate headers
    if(this->headers.find(name) != this->headers.end()) {
        if(name == "Content-Length") {
            this->headers[name] = value;
        }
        else {
            this->headers[name] += ',' + value;
        }
    }
    else {
        // New header. Add to the collection.
        this->headers[name] = value;
    }
}

const string* http_request::find_header(const std::string &key) const
{
    header_list::const_iterator it = this->headers.find(key);
    if(it != this->headers.end()) {
        return &(it->second);
    }
    else {
        return NULL;
    }
}

const string* http_request::find_query(const std::string &key) const
{
    query_list::const_iterator it = this->query.find(key);
    if(it != this->query.end()) {
        return &(it->second);
    }
    else {
        return NULL;
    }
}

void http_request::add_content(const char* data, size_t len)
{
    if(!dump_content) {
        content.append(data, len);
    }
    content_received += len;
}

u64 http_request::read_content(std::string& buf)
{
    u64 rv = 0;
    size_t bytes_read = content.size();

    rv = content_read;    
    buf.assign(content, 0, bytes_read);
    content.erase(0, bytes_read);
    content_read += bytes_read;

    return rv;
}

bool http_request::read_done() const
{
    return (dump_content || content_read == content_len);
}

bool http_request::read_complete() const
{
    return (!dump_content && content_read == content_len);
}

u64 http_request::content_length() const
{
    return content_len;
}

size_t http_request::content_available() const
{
    return content.size();
}

void http_request::purge_content()
{
    dump_content = true;
    content_read += content.size();
    content.clear();
}

void http_request::dump(std::string& out)
{
    out = http_method + " " + uri + " " + version + "\r\n";

    for(header_list::iterator it = this->headers.begin(); 
        it != this->headers.end();
        it++) {
        
        out += it->first + ": " + it->second + "\r\n";
    }
    out += "\r\n";
    out += content;
}

void http_request::dump()
{
    string out;

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                   "Dumping Request:");

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                   "%s %s %s",
                   http_method.c_str(), uri.c_str(), version.c_str());

    for(header_list::iterator it = this->headers.begin(); 
        it != this->headers.end();
        it++) {
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                       "%s: %s",
                       it->first.c_str(), it->second.c_str());
    }
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                   "<end headers>");
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                   "%s", content.c_str());
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                   "End Request.");
}

void http_request::dump_header(std::string& out)
{
    string line;

    out = http_method + " " + uri + " " + version + "\r\n";

    for(header_list::iterator it = this->headers.begin(); 
        it != this->headers.end();
        it++) {
        
        out += it->first + ": " + it->second + "\r\n";
    }
    out += "\r\n";
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

size_t http_request::receive(const char* buf, size_t len)
{
    size_t rv = 0;
    size_t beginline = 0;

    //  See http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html
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

    if(recv_state == GET_CONTENT) {
        // Collect data into the content.
        if(beginline != len &&
           content_received < content_len) {
            size_t linelen = len - beginline;
            if(linelen > (content_len - content_received)) {
                linelen = content_len - content_received;
            }
            add_content(&(buf[beginline]), linelen);
            rv += linelen;
        }

        if(content_len == content_received) {
            recv_state = GET_COMPLETE;
        }
    }

    return rv;
}

bool http_request::headers_complete() const
{
    return (recv_state == GET_CONTENT ||
            receive_complete());
}

bool http_request::receive_complete() const
{
    return (recv_state == GET_COMPLETE);
}

