/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF
 *  ACER CLOUD TECHNOLOGY INC.
 */

#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"

#ifndef IS_TS
#define IS_TS
#endif

#include "ts_client.hpp"
#include "ts_ext_server.hpp"
#include "ts_server.hpp"
#include "gvm_errors.h"
#include "ts_internal.hpp"

#include "vpl_conv.h"
#include "vpl_thread.h"
#include "vpl_th.h"
#include "vplex_math.h"
#include "echoSvc.hpp"

#include <list>
#include <string>
#include <sstream>

using namespace std;
using namespace TS;

static const u32 ECHO_TSS_READ_BUF_SIZE = 4096;
static const u32 WAIT_IF_RCVD_QUE_BYTES  = 2*1024*1024;
static const u32 CONT_IF_RCVD_QUE_BYTES  = 1*1024*1024;

static const u8 echoSvcTestCmdMsgId[] = ECHO_SVC_TEST_CMD_MSG_ID_STR;

namespace EchoSvc {

struct DataToEcho
{
    char *data;
    size_t len;

    DataToEcho () {
        data = 0;
        len = 0;
    }

    ~DataToEcho () {
        if (data) {
            delete[] data;
        }
    }
};

// struct to pass to tss_write thread
struct EchoServiceWThreadArgs
{
    TSIOHandle_t io_handle;
    VPLMutex_t *mutex;
    VPLCond_t *cond;
    list<DataToEcho> rcvd_bufs;
    u64 numQueuedBytes;
    u32 readBufSize;
    s32 testId;                  // (s32) EchoServiceTestIdEnum
    u32 clientInstanceId;
    bool isclosed;
    bool isWriteActive;
    bool isReaderWaiting;
};

#define ARRAY_LEN(x)    (sizeof(x)/sizeof(x[0]))

// Thread for server write
static VPLThread_return_t tss_write (VPLThread_arg_t arg)
{
    TSError_t err;
    string error_msg;
    EchoServiceWThreadArgs *ta = (EchoServiceWThreadArgs *)arg;
    TSIOHandle_t io_handle = ta->io_handle;
    u32 ci = ta->clientInstanceId;
    u64 numQueuedBytes;
    u32 numQueuedBuffers;
    u32 poppedQueBytes = 0;
    u32 incrementalBytesSent = 0;
    u64 totalBytesSent = 0;
    u64 logWhen_totalBytesSent = 0;
    u64 logEvery_bytesSent = 0;  // set based on VPLTrace_GetBaseLevel()
    bool dontExpectClose = false;

    switch (VPLTrace_GetBaseLevel()) {
        // There are no logs at all when not actively echoing.
        // Only large data tranfers (over 6 GB) will get more than 1 log
        // based on logEvery_bytesSent at VPLTrace_level TRACE_INFO.
        // Logs per time estimates are for echoing continuously on ubuntu vm (ymmv).
        case TRACE_FINEST:
        case TRACE_FINER:
        case TRACE_FINE:
            logEvery_bytesSent = 10*1024*1024; // < 704 logs per hour (< 12 per minute)
            break;
        case TRACE_INFO:
            // logEvery_bytesSent = 1024*1024*1024; // < 7 logs per hour (< 1.2 logs per 10 min)
            logEvery_bytesSent = 7334858760ULL; // about 1 log per hour (6.8311195448 GB)
            break;
        case TRACE_WARN:
            logEvery_bytesSent = 50*(1024*1024*1024ULL); // about 3 logs per day
            break;
        case TRACE_ERROR:
        default:
            logEvery_bytesSent = 150*(1024*1024*1024ULL); // about one log per day
            break;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
        "inst %u: Starting execution of echo service write thread for testId = %d", ci, ta->testId);

    for (;;) {
        DataToEcho buf;

        err = VPLMutex_Lock(ta->mutex);
        if (err != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: VPLMutex_Lock failed %d", ci, err);
            break;
        }

        ta->numQueuedBytes -= poppedQueBytes;
        poppedQueBytes = 0;
        numQueuedBytes = ta->numQueuedBytes;
        numQueuedBuffers = (u32) ta->rcvd_bufs.size();

        if (totalBytesSent >= logWhen_totalBytesSent) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: "
                "TSS_Write wrote %u bytes, "FMTu64" total, queued for echo: %u buffers ("FMTu64" allocated bytes)",
                 ci, incrementalBytesSent, totalBytesSent, numQueuedBuffers, numQueuedBytes);
            logWhen_totalBytesSent += logEvery_bytesSent;
        }

        // if less than n allocated bytes in queue and reader is waiting, signal
        if (ta->isReaderWaiting && numQueuedBytes < CONT_IF_RCVD_QUE_BYTES) {
            err = VPLCond_Signal(ta->cond);
            if (err != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: VPLCond_Signal failed %d", ci, err);
                break;
            }
        }

        while (ta->isclosed == false  &&  numQueuedBuffers == 0) {
            VPLCond_TimedWait(ta->cond, ta->mutex, VPL_TIMEOUT_NONE);
            numQueuedBytes = ta->numQueuedBytes;
            numQueuedBuffers = (u32) ta->rcvd_bufs.size();
        }

        if (ta->isclosed) {
            err = VPLMutex_Unlock(ta->mutex);
            if (err != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: VPLMutex_Unlock failed %d", ci, err);
            }
            break;
        }    

        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "inst %u: Echoing received data", ci);

        buf = ta->rcvd_bufs.front();
        ta->rcvd_bufs.front().data = 0;  // so pop won't delete the data
        ta->rcvd_bufs.pop_front();
        poppedQueBytes = ta->readBufSize;
        incrementalBytesSent = 0;

        err = VPLMutex_Unlock(ta->mutex);
        if (err != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: VPLMutex_Unlock failed %d", ci, err);
            break;
        }

        err = TS::TSS_Write(io_handle, buf.data, buf.len, error_msg);

        if (ta->testId == ClientCloseWhileServiceWriting) {
            if ( totalBytesSent > EchoMinBytesBeforeClientCloseWhileSvcWriting ) {
                dontExpectClose = false;
            } else {
                dontExpectClose = true;
            }
        }

        if (err != TS_OK) {
            if (err == TS_ERR_CLOSED) {
                if (dontExpectClose) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "inst %u: tss_write() didn't expect tunnel closed  %d:%s",
                         ci, err, error_msg.c_str());
                } else {
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: TSS_Write returned TS_ERR_CLOSED", ci);
                    err = TS_OK;
                }
            } else {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: tss_write() %d:%s",
                    ci, err, error_msg.c_str());
            }
            // currently exit for any error
            break;
        }

        totalBytesSent += buf.len;
        incrementalBytesSent = (u32) buf.len;

        if (ta->testId == ServiceCloseAfterWrite) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: done writing for ServiceCloseAfterWrite test", ci);
            break;
        }

        if (ta->testId == ServiceCloseWhileClientWritng) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                "inst %u: done writing for ServiceCloseWhileClientWritng test", ci);
            break;
        }
    }

    {
        int rv;

        rv = VPLMutex_Lock(ta->mutex);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: VPLMutex_Lock failed %d", ci, rv);
            if (!err) {err = rv;}
        }

        ta->isWriteActive = false;
        ta->numQueuedBytes -= poppedQueBytes;
        numQueuedBytes = ta->numQueuedBytes;
        numQueuedBuffers = (u32) ta->rcvd_bufs.size();

        rv = VPLCond_Signal(ta->cond);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: VPLCond_Signal failed %d", ci, rv);
            if (!err) {err = rv;}
        }

        rv = VPLMutex_Unlock(ta->mutex);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: VPLMutex_Unlock failed %d", ci, rv);
            if (!err) {err = rv;}
        }
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: "
        "wrote "FMTu64" total bytes, still queued for echo %u buffers ("FMTu64" allocated bytes)",
        ci, totalBytesSent, numQueuedBuffers, numQueuedBytes);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: returning %d", ci, err);
    return (VPLThread_return_t)err;
}


// Echo service handler
void handler(TSServiceRequest_t& request)
{
    int rv = 0;
    TSError_t err;
    TSError_t first_err = TS_OK;
    string error_msg;
    VPLMutex_t thread_state_mutex;
    VPLCond_t thread_state_cond;
    VPLThread_return_t thread_rv = 0;
    EchoServiceWThreadArgs args;
    EchoSvcTestCmd cmd;
    u32 expectedCmdMsgSize = (u32)cmd.expectedMsgSize();
    s32 testId = TunnelAndConnectionWorking;
    TSIOHandle_t io_handle = request.io_handle;
    u32 ci = 0;
    VPLThread_fn_t startRoutine_write = tss_write;
    VPLThread_t tss_write_thread;
    int i;
    u32 readBufSize = ECHO_TSS_READ_BUF_SIZE;
    u32 extraReadDelay = 0; // usec
    bool ignore_conn = false;
    u64 totalBytesRead = 0;
    u64 numQueuedBytes = 0;
    u32 numQueuedBuffers = 0;
    u64 logWhen_totalBytesRead = 0;
    u64 logEvery_bytesRead = 0;  // set based on VPLTrace_GetBaseLevel()
    bool pending_data_matched_ClientCloseAfterWrite = false;
    bool require_echo_cmd = false;
    bool firstMsgIsCmd = true;
    DataToEcho buf;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "log level: %s svc_handle: %p client: "FMTu64" device: "
        FMTu64" ioh: %p",
            VPLTrace_LevelStr(VPLTrace_GetBaseLevel()),
            request.service_handle, request.client_user_id, 
            request.client_device_id, request.io_handle);

    switch (VPLTrace_GetBaseLevel()) {
        // There are no logs at all when not actively echoing.
        // Only large data tranfers (over 6 GB) will get more than 1 log
        // based on logEvery_bytesSent at VPLTrace_level TRACE_INFO.
        // Logs per time estimates are for echoing continuously on ubuntu vm (ymmv).
        case TRACE_FINEST:
        case TRACE_FINER:
        case TRACE_FINE:
            logEvery_bytesRead = 10*1024*1024; // < 704 logs per hour (< 12 per minute)
            break;
        case TRACE_INFO:
            // logEvery_bytesSent = 1024*1024*1024; // < 7 logs per hour (< 1.2 logs per 10 min)
            logEvery_bytesRead = 7334858760ULL; // about 1 log per hour (6.8311195448 GB)
            break;
        case TRACE_WARN:
            logEvery_bytesRead = 50*(1024*1024*1024ULL); // about 3 logs per day
            break;
        case TRACE_ERROR:
        default:
            logEvery_bytesRead = 150*(1024*1024*1024ULL); // about one log per day
            break;
    }

    buf.data = new char[readBufSize];
    buf.len = expectedCmdMsgSize;


    err = TS::TSS_Read(io_handle, buf.data, buf.len, error_msg);
    if (err != TS_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Expected to receive echo service test command but got %d:%s",
                    err, error_msg.c_str());
        goto exit1;
    }
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "tss_read for commands result: %d, buf.len = %u", err, (u32)buf.len);

    if (buf.len != expectedCmdMsgSize) {
        if (require_echo_cmd) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                "First TSS_Read %u bytes, expected echo cmd bytes %u", (u32)buf.len, expectedCmdMsgSize);
            err = -1;
            goto exit1;
        }
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
            "First TSS_Read %u bytes, expected echo cmd bytes %u", (u32)buf.len, expectedCmdMsgSize);
        firstMsgIsCmd = false;
    }
    else if (memcmp( buf.data, echoSvcTestCmdMsgId, sizeof cmd.msgId )) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "First TSS_Read does not look like cmd from client");
        if (require_echo_cmd) {
            err = -1;
            goto exit1;
        }
        firstMsgIsCmd = false;
    }

    if (firstMsgIsCmd == false) {
        // There is no command, just echo the received data
        totalBytesRead += buf.len;
        args.rcvd_bufs.push_back(buf);
        buf.data = 0;
        args.numQueuedBytes = readBufSize;
        numQueuedBytes = args.numQueuedBytes;
        numQueuedBuffers = 1;
    }
    else {
        u32 tmp;
        char *s = buf.data + sizeof cmd.msgId;

        memcpy(&tmp, s, sizeof(u32));
        cmd.msgSize = VPLConv_ntoh_u32(tmp);
        s += sizeof(u32);

        memcpy(&tmp, s, sizeof(u32));
        cmd.extraReadDelay = VPLConv_ntoh_u32(tmp);
        s += sizeof(u32);

        memcpy(&tmp, s, sizeof(s32));
        cmd.testId = VPLConv_ntoh_s32(tmp);
        s += sizeof(s32);

        memcpy(&tmp, s, sizeof(u32));
        cmd.clientInstanceId = VPLConv_ntoh_u32(tmp);
        s += sizeof(u32);

        // A received command is not echoed
        delete[] buf.data;
        buf.data = 0;
        // Treat unknown test id as error
        testId = cmd.testId;
        if (testId > MaxEchoServiceTestId) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                "Expected cmd testId < %d, got %d", MaxEchoServiceTestId, testId);
            err = -1;
            goto exit1;
        }
        if (testId == ServiceDoNothing) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: exiting because testId = %d SeviceDoNothing", ci, testId);
            goto exit1;
        }
        // Additions to cmd msg should add to cmd.
        // Backward compatible for known testIds even if excess cmd bytes.
        // Ignore extra bytes.

        if (cmd.msgSize > expectedCmdMsgSize) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: cmd.msgSize > expectedCmdMsgSize: %u > %u", ci, cmd.msgSize, expectedCmdMsgSize);
            u32 excess = cmd.msgSize - expectedCmdMsgSize;
            while (excess) {
                buf.len = excess;
                buf.data = new char[buf.len];
                err = TS::TSS_Read(io_handle, buf.data, buf.len, error_msg);
                if ( err != TS_OK ) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "TSS_Read: %d:%s",
                                err, error_msg.c_str());
                    goto exit1;
                }
                delete[] buf.data;
                buf.data = 0;
                excess -= buf.len;
            }
        }
    }

    ci = cmd.clientInstanceId;
    ignore_conn = (testId == LostConnection) ? true : false;
    extraReadDelay = cmd.extraReadDelay;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: testId = %d cmd msg size %u, extraReadDelay %u msec, read buffer size %u", ci, testId, cmd.msgSize, extraReadDelay, readBufSize);

    if (testId == ServiceCloseWith_TS_CLOSE) {
        //expect TS_Close() called by service to be invalid.  Expect TS_ERR_BAD_HANDLE
        err = TS::TS_Close(io_handle, error_msg);
        if (err == TS_ERR_BAD_HANDLE) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: TS_Close() returned expected error err = %d:%s", ci, err, error_msg.c_str());
            err = TS_OK;
        } else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: TS_Close() expected TS_ERR_BAD_HANDLE, got = %d:%s", ci, err, error_msg.c_str());
            if (!err) {
                err = -1;
            }
            goto exit1;
        }
    }

    // Init Cond
    rv = VPLCond_Init(&thread_state_cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,  "FAIL: inst %u: VPLCond_Init() %d", ci, rv);
        first_err = rv;
        goto exit1;
    }

    // Init Mutex
    rv = VPLMutex_Init(&thread_state_mutex);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: VPLMutex_Init() %d", ci, rv);
        first_err = rv;
        goto exit2;
    }

    // Initialize shared fields
    args.io_handle = request.io_handle;
    args.mutex = &thread_state_mutex;
    args.cond = &thread_state_cond;
    args.numQueuedBytes = numQueuedBytes;
    args.readBufSize = readBufSize;
    args.testId = testId;
    args.clientInstanceId = cmd.clientInstanceId;
    args.isclosed = false;
    args.isWriteActive = false;
    args.isReaderWaiting = false;

    if (testId != ServiceDontEcho) {
        // Create thread for write on server side
        args.isWriteActive = true;
        rv = VPLThread_Create(&tss_write_thread, startRoutine_write, &args, NULL, "TSS_Write_thread");
        if (rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: Error creating thread for TSS_Write", ci);
            first_err = rv;
            args.isWriteActive = false;
            goto exit3;
        }
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: Created thread for TSS_Write", ci);
    }

    // Starting read on server side 
    for (i = 0; !first_err; ++i) {
        if (totalBytesRead >= logWhen_totalBytesRead) {

            logWhen_totalBytesRead += logEvery_bytesRead;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: received "FMTu64" total bytes to echo", ci, totalBytesRead);
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                "inst %u: queued for echo: %u buffers ("FMTu64" allocated bytes)",
                ci, numQueuedBuffers, numQueuedBytes);
        }

        if (testId == ClientCloseAfterWrite) {
            // sleep to allow close to happen after client writes, before service reads
            VPLThread_Sleep(VPLTime_FromSec(2));
        }

        buf.len = readBufSize;
        if (!buf.data) {
            buf.data = new char[buf.len];
        }

        if (testId == ServiceIgnoreIncoming && (i == 3)) {
            // Process the first 3 messages normally
            // Then sleep while client tries to write
            // and exit the service upon awakening.
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0, "tss_read: inst %u: ignoring incomming for 30 seconds", ci);
            VPLThread_Sleep(VPLTime_FromSec(30));
            err = TS_OK;
            break;
        }
        else {

            // If this flag is set trigger a lost connection after 50 accesses.
            if (ignore_conn && (i > 50)) {
                ignore_conn = false;
            }

            err = TS::TSS_Read(io_handle, buf.data, buf.len, error_msg);
        }

        if (err != TS_OK) {
            if (err == TS_ERR_CLOSED) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: tss_read returned TS_ERR_CLOSED", ci);
                if (testId == ClientCloseAfterWrite) {
                    if (!pending_data_matched_ClientCloseAfterWrite) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: "
                            "ClientCloseAfterWrite pending data not received", ci);
                        first_err = err;
                    } else {
                        // so client read close isn't because of service exit
                        VPLThread_Sleep(VPLTime_FromSec(10));
                    }
                }
                break;
            }
            else {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: tss_read %d:%s", ci, err, error_msg.c_str());
                if (true) { // currently exit the service for any error
                    first_err = err;
                    break;
                } else {
                    continue;
                }
            }
        }

        totalBytesRead += buf.len;

        if (testId == ClientCloseAfterWrite) {
            u32 i;
            if (buf.len < 1) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: "
                    "ClientCloseAfterWrite pending data not received", ci);
                first_err = -1;
                break;
            }
            for (i = 0;  i < buf.len;  ++i) {
                if ((u8)buf.data[i] != (i % 256)) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: "
                                     "ClientCloseAfterWrite pending data mismatch at byte %u", ci, i);
                    first_err = -1;
                    break;
                }
            }
            if (first_err) {
                break;
            }
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: ClientCloseAfterWrite pending data was verified", ci);
            pending_data_matched_ClientCloseAfterWrite = true;
        }

        rv = VPLMutex_Lock(args.mutex);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: VPLMutex_Lock failed %d", ci, rv);
            first_err = rv;
            break;
        }

        if (args.isWriteActive) {
            args.rcvd_bufs.push_back(buf);
            buf.data = 0;
            args.numQueuedBytes += readBufSize;  // allocated buffer queued but might be just 1 byte to echo
        }

        numQueuedBytes = args.numQueuedBytes;
        numQueuedBuffers = (u32)args.rcvd_bufs.size();

        rv = VPLCond_Signal(args.cond);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: VPLCond_Signal failed %d", ci, rv);
            first_err = rv;
            break;
        }

        if (numQueuedBytes > WAIT_IF_RCVD_QUE_BYTES) {
            while (args.isWriteActive == true  &&  numQueuedBytes > CONT_IF_RCVD_QUE_BYTES) {
                args.isReaderWaiting = true;
                VPLCond_TimedWait(args.cond, args.mutex, VPL_TIMEOUT_NONE);
                numQueuedBuffers = (u32)args.rcvd_bufs.size();
                numQueuedBytes = args.numQueuedBytes;
                args.isReaderWaiting = false;
            }
        }

        rv = VPLMutex_Unlock(args.mutex);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: VPLMutex_Unlock failed %d", ci, rv);
            first_err = rv;
            break;
        }

        if (testId == ServiceCloseAfterWrite) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                "inst %u: queued for echo: %u buffers ("FMTu64" bytes)",
                ci, numQueuedBuffers, numQueuedBytes);
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: done reading for ServiceCloseAfterWrite test, waiting for write thread to finish", ci);
            break;
        }
        
        if (testId == ServiceCloseWhileClientWritng) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: Closing server for ServiceCloseWhileClientWritng test", ci);
            break;
        }

        if ( extraReadDelay ) {
            VPLThread_Sleep(VPLTime_FromMillisec(extraReadDelay));
        }
    }
   
    err = TS_OK;  // errors at this point are indicated by first_err
    rv = VPLMutex_Lock(args.mutex);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: tss_read: VPLMutex_Lock failed %d", ci, rv);
        if (!first_err) first_err = rv;
    }

    if (testId != ServiceCloseAfterWrite || first_err) {
        args.isclosed = true;
        rv = VPLCond_Signal(args.cond);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: tss_read: VPLCond_Signal failed %d", ci, rv);
            if (!first_err) first_err = rv;
        }
    }

    rv = VPLMutex_Unlock(args.mutex);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: tss_read: VPLMutex_Unlock failed %d", ci, rv);
        if (!first_err) first_err = rv;
    }

    if (testId != ServiceDontEcho) {
        // Wait for the server write thread to finish
        rv = VPLThread_Join(&tss_write_thread, &thread_rv);
        if (rv != TS_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: VPLThread_Join tss_write_thread returned %d", ci, rv);
            if (!first_err) first_err = rv;
        } else if (thread_rv) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "inst %u: tss_write_thread returned %d", ci, (int) thread_rv);
            if (!first_err) first_err = (int) thread_rv;
        } else {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: tss_write_thread finished succesfully", ci);
        }
    }
    
exit3:
    err = TS_OK;  // errors at this point are indicated by first_err
    // Destroy Mutex
    rv = VPLMutex_Destroy(&thread_state_mutex);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: Server: VPLMutex_Destroy() %d", ci, rv);
        if (!first_err) first_err = rv;
    }

exit2:
    err = TS_OK;  // errors at this point are indicated by first_err
    // Destroy Cond
    rv = VPLCond_Destroy(&thread_state_cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL: inst %u: VPLCond_Destroy() %d", ci, rv);
        if (!first_err) first_err = rv;
    }
   
exit1:
    // either err or first_err might be set
    if (first_err) {
        err = first_err;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: received "FMTu64" total bytes to echo", ci, totalBytesRead);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
        "inst %u: svc_handle: %p client: "FMTu64" device: "
            FMTu64" ioh: %p - exiting with status %d", ci,
            request.service_handle, request.client_user_id, 
            request.client_device_id, request.io_handle, err);

    if (err != TS_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "TC_RESULT = FAIL ;;; TC_NAME = TsTest");
        // FAILED_ASSERT("inst %u: echo_svc_handler exiting with error %d", ci, err);
    }
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "inst %u: exiting", ci);
}

} // namespace EchoSvc
