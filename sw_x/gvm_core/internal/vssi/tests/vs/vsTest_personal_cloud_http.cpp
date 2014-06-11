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

// Perform a series of tests for the VSS (personal cloud) HTTP interface
// used by OPS.
// * Upload a zero-byte file
// * Read a zero-byte file
// * Upload a small file
// * Read a small file
// * Upload a large file
// * Read a large file
// * Create a directory
// * Read a directory
// * Rename a file
// * Delete a file
// * Delete a directory
// * Delete the root directory (delete all data)
// * Read root directory (confirm deletion)
//
// Each test is canned, sending a pre-composed HTTP request to the server and
// waiting for response or connection lost. A test passes if the HTTP result
// is as expected and the body data is as expected.

#include "vsTest_personal_cloud_http.hpp"

#include "vplex_trace.h"
#include "vpl_socket.h"
#include "vplex_serialization.h"

#include <sstream>
#include <vector>

#include <stdlib.h>

using namespace std;

extern const char* vsTest_curTestName;

static const string USER_KEY = "%USER%";
static const string DATASET_KEY = "%DATASET%";
static const string FILE_KEY = "%FILE%";
static const string HOST_KEY = "%HOST%";
static const string LENGTH_KEY = "%LENGTH%";
static const string DEST_KEY = "%DEST%";
static const string SESSIONHANDLE_KEY = "%SESSIONHANDLE%";
static const string SERVICETICKET_KEY = "%SERVICETICKET%";

static const string downloadQuery =
    "GET /GET/%USER%/%DATASET%/%FILE% HTTP/1.1\r\n"
    "Host: %HOST%\r\n"
    "x-session-handle: %SESSIONHANDLE%\r\n"
    "x-service-ticket: %SERVICETICKET%\r\n"
    "x-all-attributes: yes\r\n"
    "\r\n";
static const string downloadRangeQuery =
    "GET /GET/%USER%/%DATASET%/%FILE% HTTP/1.1\r\n"
    "Host: %HOST%\r\n"
    "x-session-handle: %SESSIONHANDLE%\r\n"
    "x-service-ticket: %SERVICETICKET%\r\n"
    "Range: bytes=10-\r\n"
    "\r\n";
static const string uploadFixedQuery =
    "PUT /PUT/%USER%/%DATASET%/%FILE% HTTP/1.1\r\n"
    "Host: %HOST%\r\n"
    "Content-Length:%LENGTH%\r\n"
    "x-session-handle: %SESSIONHANDLE%\r\n"
    "x-service-ticket: %SERVICETICKET%\r\n"
    "\r\n";
static const string makeDirQuery =
    "PUT /MKCOL/%USER%/%DATASET%/%FILE% HTTP/1.1\r\n"
    "Host: %HOST%\r\n"
    "x-session-handle: %SESSIONHANDLE%\r\n"
    "x-service-ticket: %SERVICETICKET%\r\n"
    "\r\n";
static const string renameQuery =
    "PUT /MOVE/%USER%/%DATASET%/%FILE% HTTP/1.1\r\n"
    "Host: %HOST%\r\n"
    "Destination: %DEST%\r\n"
    "x-session-handle: %SESSIONHANDLE%\r\n"
    "x-service-ticket: %SERVICETICKET%\r\n"
    "\r\n";
static const string copyQuery =
    "PUT /COPY/%USER%/%DATASET%/%FILE% HTTP/1.1\r\n"
    "Host: %HOST%\r\n"
    "Destination: %DEST%\r\n"
    "x-session-handle: %SESSIONHANDLE%\r\n"
    "x-service-ticket: %SERVICETICKET%\r\n"
    "\r\n";
static const string removeQuery =
    "DELETE /DELETE/%USER%/%DATASET%/%FILE% HTTP/1.1\r\n"
    "Host: %HOST%\r\n"
    "x-session-handle: %SESSIONHANDLE%\r\n"
    "x-service-ticket: %SERVICETICKET%\r\n"
    "\r\n";
static const string removeTrashQuery =
    "DELETE /TRASH/%USER%/%DATASET%/%FILE% HTTP/1.1\r\n"
    "Host: %HOST%\r\n"
    "x-session-handle: %SESSIONHANDLE%\r\n"
    "x-service-ticket: %SERVICETICKET%\r\n"
    "\r\n";

static void http_encode(string& source)
{
    size_t pos;
    char hexval[4];
    string safe_chars =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "$-_.!*'(),/";

    pos = source.find_first_not_of(safe_chars);
    while(pos != string::npos) {
        if(source[pos] == '+') {
            // " " => "+"
            source.replace(pos, 1, 1, '+');
            pos = source.find_first_not_of(safe_chars, pos + 1);
        }
        else {
            // others => "%##"
            snprintf(hexval, 3, "%%%02X", source[pos]);
            source.replace(pos, 1, hexval, 3);
            pos = source.find_first_not_of(safe_chars, pos + 3);
        }
    }
}

static void replace_user(string& source, u64 user_id)
{
    size_t pos = source.find(USER_KEY);
    if(pos != string::npos) {
        stringstream userId;
        userId << hex << uppercase << user_id;
        source.replace(pos, USER_KEY.size(),
                       userId.str());
    }
}

static void replace_dataset(string& source, u64 dataset_id)
{
    size_t pos = source.find(DATASET_KEY);
    if(pos != string::npos) {
        stringstream datasetId;
        datasetId << hex << uppercase << dataset_id;
        source.replace(pos, DATASET_KEY.size(),
                       datasetId.str());
    }
}

static void replace_file(string& source, const string& filename)
{
    size_t pos = source.find(FILE_KEY);
    if(pos != string::npos) {
        string encoded_file = filename;
        http_encode(encoded_file);
        source.replace(pos, FILE_KEY.size(),
                       encoded_file);
    }
}

static void replace_dest(string& source, const string& filename)
{
    size_t pos = source.find(DEST_KEY);
    if(pos != string::npos) {
        string encoded_file = filename;
        http_encode(encoded_file);
        source.replace(pos, DEST_KEY.size(),
                       encoded_file);
    }
}

static void replace_host(string& source, const string& host)
{
    size_t pos = source.find(HOST_KEY);
    if(pos != string::npos) {
        source.replace(pos, HOST_KEY.size(),
                       host);
    }
}

static void replace_length(string& source, u64 filelen)
{
    size_t pos = source.find(LENGTH_KEY);
    if(pos != string::npos) {
        stringstream length;
        length << filelen;
        source.replace(pos, LENGTH_KEY.size(),
                       length.str());
    }
}

static void replace_sessionhandle(string& source, u64 handle)
{
    size_t pos = source.find(SESSIONHANDLE_KEY);
    if(pos != string::npos) {
        stringstream handlestr;
        handlestr << hex << uppercase << handle;
        source.replace(pos, SESSIONHANDLE_KEY.size(),
                       handlestr.str());
    }
}

static void replace_serviceticket(string& source, const string& ticket)
{
    size_t pos = source.find(SERVICETICKET_KEY);
    if(pos != string::npos) {
        // Must base64 encode the ticket.
        size_t encoded_len = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(ticket.length());
        char encoded[encoded_len];
        VPL_EncodeBase64(ticket.c_str(), ticket.length(),
                         encoded, &encoded_len, false, false);
        source.replace(pos, SERVICETICKET_KEY.size(),
                       encoded, encoded_len);
    }
}

static int sendall(VPLSocket_t sockfd,
                   const char* buffer,
                   size_t length)
{
    size_t so_far = 0;
    int rc;

    do {
        rc = VPLSocket_Send(sockfd, buffer + so_far,
                            length - so_far);
        if(rc > 0) {
            so_far += rc;
        }
        else if(rc == VPL_ERR_AGAIN) {
            // Wait up to 5 minutes for ready to send, then give up.
            VPLSocket_poll_t psock;
            psock.socket = sockfd;
            psock.events = VPLSOCKET_POLL_OUT;
            rc = VPLSocket_Poll(&psock, 1, VPLTIME_FROM_SEC(300));
            if(rc < 0) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Encountered error %d waiting for socket ready to send.",
                                 rc);
                break;
            }
            if(psock.revents != VPLSOCKET_POLL_OUT) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Encountered unexpected event %x waiting for socket ready to send.",
                                 psock.revents);
                break;
            }
        }
        else {
            break;
        }
    } while(so_far < length);

    if(so_far == length) {
        return so_far;
    }
    else {
        return -1;
    }
}

static const string randomData =
    "This is a test string. Repeat it as often as you wish to fill any file."
    " Some garbage characters: !@#$%^&*(_-+={[|\\:;<,>.?/~`\"\'}])([{"
    " Some numbers 1234567890..................\n";

static int pcHttp_sendQuery(VPLSocket_t sockfd,
                            const std::string& query,
                            u64 handle,
                            const std::string& ticket,
                            u64 user_id,
                            u64 dataset_id,
                            const std::string& host,
                            const std::string& filename,
                            const std::string& destination,
                            u64 bodylen)
{
    int rv = -1;
    u64 lineno = 0;

    // Substitute wildcards into the query.
    string req_header = query;
    replace_user(req_header, user_id);
    replace_dataset(req_header, dataset_id);
    replace_file(req_header, filename);
    replace_host(req_header, host);
    replace_length(req_header, bodylen);
    replace_dest(req_header, destination);
    replace_sessionhandle(req_header, handle);
    replace_serviceticket(req_header, ticket);

    // Send header
    rv = sendall(sockfd, req_header.data(), req_header.length());
    if(rv != req_header.length()) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Failed to send %d bytes of query on socket "FMT_VPLSocket_t": %d.\n%s",
                         req_header.length(), VAL_VPLSocket_t(sockfd),
                         rv, req_header.c_str());

        goto exit;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Sending request:\n%s",
                        req_header.c_str());

    // If body length is greater than 0, send the body data.
    while(bodylen > 0) {
        string outline;
        {
            stringstream outlinestream;
            outlinestream << lineno << ": " << randomData;
            outline = outlinestream.str();
        }

        size_t to_send = outline.length();
        if(to_send > bodylen) {
            to_send = bodylen;
        }
        rv = sendall(sockfd, outline.data(), to_send);
        if(rv != to_send) {
            goto exit;
        }

        bodylen -= to_send;
        lineno++;
    }

    rv = 0;
 exit:
    return rv;
}

static int pcHttp_ReceiveResponse(VPLSocket_t sockfd,
                                  int expect,
                                  u64 expectlen,
                                  u64& received)
{
    int rv = -1; // failed until proven to pass
    size_t buflen = 8192;
    char buf[buflen];
    size_t beginline = 0;
    size_t linelen = 0;
    int rc = 0;
    int i;

    string line;
    u64 lineno = 0;
    string response;
    int respcode = 0;
    vector<string> headers;
    vector<string>::iterator it;
    bool done_headers = false; // true when last header received.
    u64 bodylen = 0;
    u64 offset = 0;

    received = 0;

    // Receive response line and all headers.
    do {
        // Receive a chunk of data
        if(beginline == rc) {
            rc = VPLSocket_Recv(sockfd, buf, buflen);
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "VPLSocket_Recv = %d", rc);
            if(rc == VPL_ERR_AGAIN || rc == VPL_ERR_INTR) {
                // Wait for ready to receive.
                VPLSocket_poll_t psock;
                psock.socket = sockfd;
                psock.events = VPLSOCKET_POLL_RDNORM;
                rc = VPLSocket_Poll(&psock, 1, VPLTIME_FROM_SEC(30000));
                if(rc < 0) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Encountered error %d waiting for socket ready to recv.",
                                     rc);
                    break;
                }
                if(rc != 1) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Receive timed out.");
                    break;
                }
                if(psock.revents != VPLSOCKET_POLL_RDNORM) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Encountered unexpected event %x waiting for socket ready to recv.",
                                     psock.events);
                    break;
                }

                // trying again.
                rc = 0;
            }
            else if(rc <= 0) {
                // Socket closed or had some other error.
                if(rc == 0) {
                    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                                      "Got socket closed for connection "FMT_VPLSocket_t".",
                                      VAL_VPLSocket_t(sockfd));
                }
                else {
                    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                                      "Got socket error %d for connection "FMT_VPLSocket_t".",
                                      rc, VAL_VPLSocket_t(sockfd));
                }
                break;
            }

            beginline = 0;
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Received %d bytes.", rc);
        }

        while(beginline < rc) {
            linelen = 0;
            for(i = beginline; i < rc; i++) {
                linelen++;
                if(buf[i] == '\n') {
                    break;
                }
            }

            // Add line fragment to partial line.
            line.append(buf + beginline, linelen);
            beginline += linelen;

            if(line[line.size() - 1] != '\n') {
                // Not a complete line. Get more data.
                break;
            }

            // Strip newline characters from the end.
            line.erase(line.find_first_of("\r\n"));

            if(response.empty()) {
                // Collect response line
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "Response: %s", line.c_str());
                response = line;
                line.erase();
            }
            else {
                if(line.empty()) {
                    // Blank line after headers indicates end of headers.
                    done_headers = true;
                    break;
                }
                else {
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "Header: %s", line.c_str());
                    headers.push_back(line);
                    line.erase();
                }
            }
        }

    } while(!done_headers);

    // Buffer now has only body data from beginline thru (rc - beginline)

    // Get the response code.
    linelen = response.find_first_of(' ');
    if (linelen == string::npos) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected pattern in response \"%s\"", response.c_str());
        rv = -1;
    }
    respcode = atoi(&(response[linelen + 1]));
    if(respcode == expect) {
        // passing so far.
        rv = 0;
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Response had code %d. Expected %d.",
                         respcode, expect);
    }

    // Get the body length from the headers.
    for(it = headers.begin(); it != headers.end(); it++) {
        if(it->find("Content-Length:") != string::npos) {
            bodylen = strtoull(&((*it)[it->find_first_not_of(' ', it->find_first_of(':') + 1)]), 0, 10);
        }
    }
    if(expectlen != 0 && bodylen != expectlen) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Response body is "FMTu64" bytes. "FMTu64" bytes expected.",
                         bodylen, expectlen);
        // back to failing
        rv = -1;
    }

    // If expected body length is nonzero, will verify body content received
    // has same length and matches the repeated randomData buffer.
    while(bodylen > 0) {
        if(beginline == rc) {
            rc = VPLSocket_Recv(sockfd, buf,
                                (bodylen < buflen) ? bodylen : buflen);
            if(rc == VPL_ERR_AGAIN || rc == VPL_ERR_INTR) {
                // Wait for ready to receive.
                VPLSocket_poll_t psock;
                psock.socket = sockfd;
                psock.events = VPLSOCKET_POLL_RDNORM;
                rc = VPLSocket_Poll(&psock, 1, VPLTIME_FROM_SEC(5));
                if(rc < 0) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Encountered error %d waiting for socket ready to recv.",
                                     rc);
                    rv = -1;
                    break;
                }
                if(rc != 1) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Receive timed out.");
                    rv = -1;
                    break;
                }
                if(psock.revents != VPLSOCKET_POLL_RDNORM) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Encountered unexpected event %x waiting for socket ready to recv.",
                                     psock.events);
                    rv = -1;
                    break;
                }

                // trying again.
                rc = 0;
            }
            else if(rc <= 0) {
                // Socket closed or had some other error.
                if(rc == 0) {
                    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                                      "Got socket closed for connection "FMT_VPLSocket_t".",
                                      VAL_VPLSocket_t(sockfd));
                }
                else {
                    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                                      "Got socket error %d for connection "FMT_VPLSocket_t".",
                                      rc, VAL_VPLSocket_t(sockfd));
                }
                break;
            }

            beginline = 0;
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Received %d bytes.", rc);
        }

        bodylen -= (rc - beginline);
        received += (rc - beginline);
        // Compare the data to expectations if relevant.
        if(expectlen != 0 && rv == 0) {
            while(beginline < rc) {
                string expectline;
                {
                    stringstream expectlinestream;
                    expectlinestream << lineno << ": " << randomData;
                    expectline = expectlinestream.str();
                }
                if(line.length() < expectline.length()) {
                    linelen = expectline.length() - line.length();
                    if(linelen > (rc - beginline)) {
                        linelen = rc - beginline;
                    }
                    line.append(buf + beginline, linelen);
                    beginline += linelen;
                }

                if(line.length() == expectline.length()) {
                    if(expectline.compare(line) != 0) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                         "Data mis-compared at offset %llu!\n"
                                         "Got:     \n%s\n"
                                         "Expected:\n%s\n",
                                         offset,
                                         line.c_str(), expectline.c_str());
                        rv = -1;
                    }
                    offset += line.length();
                    line.erase();
                    lineno++;
                }
            }
        }
        else {
            // If body is short (4k or less),
            // Dump the data as it's received for manual inspection.
            if((bodylen + received) <= 4096) {
                VPLTRACE_DUMP_BUF_ALWAYS(TRACE_BVS, 0,
                                         buf + beginline,
                                         rc - beginline);
            }
            beginline = rc;
        }

    } // while receiving body

    // Compare the data to expectations if relevant.
    if(expectlen != 0 && rv == 0 &&
       !line.empty()) {
        string expectline;
        {
            stringstream expectlinestream;
            expectlinestream << lineno << ": " << randomData;
            expectline = expectlinestream.str();
        }

        if(expectline.compare(0, line.length(), line) != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Data mis-compared at offset %llu!\n"
                             "Got:(%d) \n%s\n"
                             "Expected:\n%s\n",
                             offset,
                             line.length(), line.c_str(), expectline.c_str());
            rv = -1;
        }
    }

    return rv;
}

static int pcHttp_upload(VPLSocket_t sockfd,
                         u64 handle,
                         const std::string& ticket,
                         u64 user_id,
                         u64 dataset_id,
                         const std::string& host,
                         const std::string& filename,
                         u64 size)
{
    int rv = 0;
    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t end;
    u64 received;

    // Send upload query.
    rv = pcHttp_sendQuery(sockfd, uploadFixedQuery, handle, ticket,
                          user_id, dataset_id, host, filename, /*dummy*/filename, size);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Send upload file %s for "FMTu64" bytes failed.",
                         filename.c_str(), size);
        goto exit;
    }

    // Receive response.
    rv = pcHttp_ReceiveResponse(sockfd, 201, 0, received);

    end = VPLTime_GetTimeStamp();
    if(end == start) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Uploaded "FMTu64" bytes in 0us. No Rate calculable",
                            size);
    }
    else {
        u64 mbps = (size * 8)/ (end - start);

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Took "FMT_VPLTime_t"us to upload "FMTu64" bytes ("FMTu64"Mb/s).",
                            end - start, size, mbps);
    }

 exit:
    return rv;
}

static int pcHttp_downloadAndVerify(VPLSocket_t sockfd,
                                    u64 handle,
                                    const std::string& ticket,
                                    u64 user_id,
                                    u64 dataset_id,
                                    const std::string& host,
                                    const std::string& filename,
                                    u64 size)
{
    int rv = 0;
    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t end;
    u64 received;

    // Send read query.
    rv = pcHttp_sendQuery(sockfd, downloadQuery, handle, ticket,
                          user_id, dataset_id, host, filename, /*dummy*/filename, 0);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Send download file %s for "FMTu64" bytes failed.",
                         filename.c_str(), size);
        goto exit;
    }

    // Receive response and verify body data.
    rv = pcHttp_ReceiveResponse(sockfd, 200, size, received);

    end = VPLTime_GetTimeStamp();
    if(end == start) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Downloaded "FMTu64" bytes in 0us. No Rate calculable",
                            received);
    }
    else {
        u64 mbps = (received * 8)/ (end - start);

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Took "FMT_VPLTime_t"us to download "FMTu64" bytes ("FMTu64"Mb/s).",
                            end - start, received, mbps);
    }

 exit:
    return rv;
}

static int pcHttp_download(VPLSocket_t sockfd,
                           u64 handle,
                           const std::string& ticket,
                           u64 user_id,
                           u64 dataset_id,
                           const std::string& host,
                           const std::string& filename)
{
    int rv = 0;
    u64 received;

    // Send read query.
    rv = pcHttp_sendQuery(sockfd, downloadQuery, handle, ticket,
                          user_id, dataset_id, host, filename, /*dummy*/filename, 0);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Send download file %s failed.",
                         filename.c_str());
        goto exit;
    }

    // Receive response and verify body data.
    rv = pcHttp_ReceiveResponse(sockfd, 200, 0, received);

 exit:
    return rv;
}

static int pcHttp_downloadRange(VPLSocket_t sockfd,
                                u64 handle,
                                const std::string& ticket,
                                u64 user_id,
                                u64 dataset_id,
                                const std::string& host,
                                const std::string& filename)
{
    int rv = 0;
    u64 received;

    // Send read query.
    rv = pcHttp_sendQuery(sockfd, downloadRangeQuery, handle, ticket,
                          user_id, dataset_id, host, filename, /*dummy*/filename, 0);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Send download file %s failed.",
                         filename.c_str());
        goto exit;
    }

    // Receive response and verify body data.
    rv = pcHttp_ReceiveResponse(sockfd, 206, 0, received);

 exit:
    return rv;
}

static int pcHttp_mkdir(VPLSocket_t sockfd,
                        u64 handle,
                        const std::string& ticket,
                        u64 user_id,
                        u64 dataset_id,
                        const std::string& host,
                        const std::string& dirname)
{
    int rv = 0;
    u64 received;

    // Send read query.
    rv = pcHttp_sendQuery(sockfd, makeDirQuery, handle, ticket,
                          user_id, dataset_id, host, dirname, /*dummy*/dirname, 0);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Send mkdir %s failed.",
                         dirname.c_str());
        goto exit;
    }

    // Receive response and verify body data.
    rv = pcHttp_ReceiveResponse(sockfd, 201, 0, received);

 exit:
    return rv;
}

static int pcHttp_rename(VPLSocket_t sockfd,
                         u64 handle,
                         const std::string& ticket,
                         u64 user_id,
                         u64 dataset_id,
                         const std::string& host,
                         const std::string& source,
                         const std::string& dest)
{
    int rv = 0;
    u64 received;

    // Send read query.
    rv = pcHttp_sendQuery(sockfd, renameQuery, handle, ticket,
                          user_id, dataset_id, host, source, dest, 0);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Send rename %s to %s failed.",
                         source.c_str(), dest.c_str());
        goto exit;
    }

    // Receive response and verify body data.
    rv = pcHttp_ReceiveResponse(sockfd, 204, 0, received);

 exit:
    return rv;
}

static int pcHttp_copy(VPLSocket_t sockfd,
                       u64 handle,
                       const std::string& ticket,
                       u64 user_id,
                       u64 dataset_id,
                       const std::string& host,
                       const std::string& source,
                       const std::string& dest)
{
    int rv = 0;
    u64 received;

    // Send read query.
    rv = pcHttp_sendQuery(sockfd, copyQuery, handle, ticket,
                          user_id, dataset_id, host, source, dest, 0);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Send copy %s to %s failed.",
                         source.c_str(), dest.c_str());
        goto exit;
    }

    // Receive response and verify body data.
    rv = pcHttp_ReceiveResponse(sockfd, 204, 0, received);

 exit:
    return rv;
}

static int pcHttp_delete(VPLSocket_t sockfd,
                       u64 handle,
                       const std::string& ticket,
                       u64 user_id,
                       u64 dataset_id,
                       const std::string& host,
                       const std::string& target)
{
    int rv = 0;
    u64 received;

    // Send read query.
    rv = pcHttp_sendQuery(sockfd, removeQuery, handle, ticket,
                          user_id, dataset_id, host, target, target/*dummy*/, 0);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Send delete %s failed.",
                         target.c_str());
        goto exit;
    }

    // Receive response and verify body data.
    rv = pcHttp_ReceiveResponse(sockfd, 204, 0, received);

 exit:
    return rv;
}

static const char vsTest_http_dataset_access [] = "Personal Cloud HTTP Dataset Access Test";
int test_http_dataset_access(const vplex::vsDirectory::DatasetDetail& dataset,
                             u64 uid, u64 handle, const std::string& ticket)
{
    int rv = 0;
    VPLSocket_t socket = VPLSOCKET_INVALID;
    VPLSocket_addr_t address;

    vsTest_curTestName = vsTest_http_dataset_access;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    // Open a connection to the server
    address.addr = VPLNet_GetAddr(dataset.storageclusterhostname().c_str());
    address.port = VPLNet_port_hton(dataset.storageclusterport());
    socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, true);
    if(VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to open socket to a server.");
        rv = -1;
        goto exit;
    }
    if(VPLSocket_Connect(socket, &address, sizeof(address)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to connect to server %x:%d on socket "FMT_VPLSocket_t".",
                         address.addr, address.port, VAL_VPLSocket_t(socket));
        VPLSocket_Close(socket);
        socket = VPLSOCKET_INVALID;
        // Allow test to be skipped on inability to connect.
        //rv = -1;
        goto exit;
    }

    // * Read root directory (to see initial condition)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Upload a small file
    rv += pcHttp_upload(socket, handle, ticket, uid, dataset.datasetid(), dataset.storageclusterhostname().c_str(), "smallFile", 50);
    if(rv != 0) {
        goto connection_lost;
    }

    // * Read root directory (to determine when server ready to download)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Read the file
    rv += pcHttp_downloadAndVerify(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "smallFile", 50);
    if(rv != 0) {
        goto connection_lost;
    }

    // * Upload a zero-byte file
    rv += pcHttp_upload(socket, handle, ticket, uid, dataset.datasetid(), dataset.storageclusterhostname().c_str(), "zeroFile", 0);
    if(rv != 0) {
        goto connection_lost;
    }

    // * Read root directory (to determine when server ready to download)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Read the zero-byte
    rv += pcHttp_downloadAndVerify(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "zeroFile", 0);
    if(rv != 0) {
        goto connection_lost;
    }

    // * Upload a larger file (10k)
    rv += pcHttp_upload(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "mediumFile", 10*1024);
    if(rv != 0) {
        goto connection_lost;
    }

    // * Read root directory (to determine when server ready to download)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Read the file
    rv += pcHttp_downloadAndVerify(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "mediumFile", 10*1024);
    if(rv != 0) {
        goto connection_lost;
    }

    // * Read range of the file
    rv += pcHttp_downloadRange(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "mediumFile");
    if(rv != 0) {
        goto connection_lost;
    }

    // * Overwrite a medium file (50k)
    rv += pcHttp_upload(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "mediumFile", 50*1024);
    if(rv != 0) {
        goto connection_lost;
    }

    // * Read root directory (to determine when server ready to download)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Read the file
    rv += pcHttp_downloadAndVerify(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "mediumFile", 50*1024);
    if(rv != 0) {
        goto connection_lost;
    }

    // * Upload a large file (50M)
    rv += pcHttp_upload(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "largeFile", 50*1024*1024);
    if(rv != 0) {
        goto connection_lost;
    }

    // * Read root directory (to determine when server ready to download)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Read a large file
    rv += pcHttp_downloadAndVerify(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "largeFile", 50*1024*1024);
    if(rv != 0) {
        goto connection_lost;
    }

#if 0
    // * Upload a super-large file (5G)
    rv += pcHttp_upload(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "superlargeFile", 0x140000000ull /*5GB*/);
    if(rv != 0) {
        goto connection_lost;
    }

    // * Read root directory (to determine when server ready to download)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Read a super-large file
    rv += pcHttp_downloadAndVerify(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "superlargeFile", 0x140000000ull /*5GB*/);
    if(rv != 0) {
        goto connection_lost;
    }
#endif

    // * Create a directory
    rv += pcHttp_mkdir(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "a_directory");

    // * Read root directory (to see if operation was successful)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Rename a file
    rv += pcHttp_rename(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "smallFile", "a_directory/smallFile");

    // * Read "a_directory" directory (to see if operation was successful)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "a_directory");

    // * Read root directory (to see if operation was successful)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Copy a file
    rv += pcHttp_copy(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "a_directory/smallFile", "otherFile");

    // * Read root directory (to see if operation was successful)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Delete a file
    rv += pcHttp_delete(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "otherFile");

    // * Read root directory (to see if operation was successful)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Delete a directory
    rv += pcHttp_delete(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "a_directory");

    // * Read root directory (to see if operation was successful)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Delete the root directory (delete all data)
    rv += pcHttp_delete(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str(), "");

    // * Read root directory (confirm deletion)
    rv += pcHttp_download(socket, handle, ticket, uid,  dataset.datasetid(), dataset.storageclusterhostname().c_str() ,"");

 connection_lost:
    if(!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLSocket_Close(socket);
    }
 exit:
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Finished test: %s. Result: %s",
                        vsTest_curTestName,
                        (rv == 0) ? "PASS" : "FAIL");
    return rv;
}
