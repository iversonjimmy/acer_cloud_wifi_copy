/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */


#include <vpl_net.h>
#include "vplex_trace.h"
#include "vplex_assert.h"

#include "vpl_conv.h"
#include "vpl_thread.h"
#include "vpl_th.h"
#include "vplex_math.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "dx_common.h"
#include "ccdconfig.hpp"
#include "ts_test.hpp"
#include "echoSvc.hpp"
#include "ts_ext_client.hpp"
#include "gvm_errors.h"

#if defined(WIN32)
#include <getopt_win.h>
#else
#include <getopt.h>
#endif

#include <list>
#include <string>
#include <sstream>

#include "log.h"

using namespace std;
using namespace EchoSvc;


static const u8 echoSvcTestCmdMsgId[] = ECHO_SVC_TEST_CMD_MSG_ID_STR;

struct DataToEcho
{
    char *data;
    size_t len;
    size_t lenCompared;

    DataToEcho () {
        data = 0;
        len = 0;
        lenCompared = 0;
    }

    ~DataToEcho () {
        if (data) {
            delete[] data;
        }
    }
};

// struct to pass to ts_read and ts_write threads
struct EchoClientRWThreadArgs
{
    const TSTestParameters *testParameters;
    TSIOHandle_t io_handle;
    VPLMutex_t *mutex;
    VPLCond_t *cond;
    list<DataToEcho> sent_bufs;
    u64 numQueuedBytes;
    u32 clientInstanceId;
    bool isdone;
    bool isReadActive;
    bool isWriterWaiting;
    bool isTunnelClosed;
};

struct EchoClientThreadArgs
{
    const TSTestParameters *testParameters;
    u32 clientInstanceId;
};




static const u32 sentQueueWaitLimit  = 2*1024*1024;
static const u32 sentQueueContLimit  = 1*1024*1024;

static const u32 maxClients = 10;


// Thread for client write
static VPLThread_return_t ts_write (VPLThread_arg_t arg)
{
    TSError_t err;
    TSError_t first_err = TS_OK;
    string error_msg;
    EchoClientRWThreadArgs *ta = (EchoClientRWThreadArgs *)arg;
    const TSTestParameters& test = *ta->testParameters;
    s32 testId = test.testId;
    u32 t = ta->clientInstanceId;
    size_t chunkSize =  test.xfer_size;
    u32 numChunks = test.xfer_cnt;
    u32 totalChunks = numChunks;
    u32 chunksToWrite = totalChunks;
    u64 totalBytes = totalChunks * chunkSize;
    u64 totalBytesSent = 0;
    u64 totalBytesRemaining = totalBytes - totalBytesSent;
    u64 logWhen_totalBytesSent = 0;
    u64 logEvery_bytesSent = 0;  // set based on logEnableLevel
    bool last_write = false;
    bool unexpectedReaderExit = false;
    bool tunnelWasClosed = false;
    u32 numChunksWritten;

    switch (test.logEnableLevel) {
        // There are no logs at all when not actively echoing.
        // Only large data tranfers (over 6 GB) will get more than 1 log
        // based on logEvery_bytesSent at the default logEnableLevel.
        // Logs per time estimates are for echoing continuously on ubuntu vm (ymmv).
        case LOG_LEVEL_TRACE:
        case LOG_LEVEL_DEBUG:
            logEvery_bytesSent = 10*1024*1024; // < 704 logs per hour (< 12 per minute)
            break;
        case LOG_LEVEL_INFO:
            // logEvery_bytesSent = 1024*1024*1024; // < 7 logs per hour (< 1.2 logs per 10 min)
            logEvery_bytesSent = 7334858760ULL; // about 1 log per hour (6.8311195448 GB)
            break;
        case LOG_LEVEL_WARN:
            logEvery_bytesSent = 50*1024*1024*1024ULL; // about 3 logs per day
            break;
        case LOG_LEVEL_ERROR:
        case LOG_LEVEL_CRITICAL:
        case LOG_LEVEL_ALWAYS:
        default:
            logEvery_bytesSent = 150*1024*1024*1024ULL; // about one log per day
            break;
    }

    // send the echo svc test cmd
    {
        EchoSvcTestCmd cmd;
        DataToEcho buf;
        char* d;

        buf.len = cmd.msgWriteSize();
        buf.data = new char[buf.len];

        memcpy (cmd.msgId, echoSvcTestCmdMsgId, sizeof cmd.msgId);
        cmd.msgSize = VPLConv_hton_u32(buf.len);
        cmd.extraReadDelay = VPLConv_hton_u32(test.server_read_delay);
        cmd.testId = VPLConv_hton_s32(testId);
        cmd.clientInstanceId = VPLConv_hton_u32(t);

        d = buf.data;
        memcpy(d, cmd.msgId, sizeof cmd.msgId);
        d += sizeof cmd.msgId;
        memcpy(d, &cmd.msgSize, sizeof cmd.msgSize);
        d += sizeof cmd.msgSize;
        memcpy(d, &cmd.extraReadDelay, sizeof cmd.extraReadDelay);
        d += sizeof cmd.extraReadDelay;
        memcpy(d, &cmd.testId, sizeof cmd.testId);
        d += sizeof cmd.testId;
        memcpy(d, &cmd.clientInstanceId, sizeof cmd.clientInstanceId);
        d += sizeof cmd.clientInstanceId;

        ASSERT( (d - buf.data) == buf.len );

        LOG_INFO ("inst %u testId %d: Starting thread", t, testId);

        // The cmd is not echoed back, so don't need to pass to read thread

        err = TS_EXT::TS_Write(ta->io_handle, buf.data, buf.len, error_msg);
        if ( err != TS_OK ) {
            LOG_ERROR("ts_write() %d:%s", err, error_msg.c_str());
            first_err = err;
            goto exit;
        }
        LOG_DEBUG ("inst %u: Wrote "FMTu_size_t" command bytes.", t, buf.len);
        VPLThread_Sleep(VPLTime_FromSec(1));  // give service time to digest cmd
    }

    for ( chunksToWrite = totalChunks;  chunksToWrite > 0;  --chunksToWrite ) {
        DataToEcho buf;
        buf.len = chunkSize;
        buf.data = new char[buf.len];
        char* dataToWrite = 0;

        if (chunksToWrite == 1) {
            last_write = true;
        }

        if ( testId != ServiceDontEcho ) {
            for (size_t i = 0; i < buf.len; ++i) {
                if (testId == ClientCloseAfterWrite) {
                    buf.data[i] = (char) i;
                } else {
                    uint32_t rand_no = VPLMath_Rand();
                    size_t n = sizeof rand_no;
                    if ( (i + n) > buf.len ) {
                        n = buf.len -i;
                    }
                    memcpy(buf.data+i, &rand_no, n);
                    i += (n - 1);
                }
            }
        }

        err = VPLMutex_Lock(ta->mutex);
        if (err != VPL_OK) {
            LOG_ERROR("inst %u: VPLMutex_Lock failed %d", t, err);
            first_err = err;
            goto exit;
        }

        if (ta->numQueuedBytes > sentQueueWaitLimit) {
            while (ta->isReadActive  &&  ta->numQueuedBytes > sentQueueContLimit) {
                ta->isWriterWaiting = true;
                VPLCond_TimedWait(ta->cond, ta->mutex, VPL_TIMEOUT_NONE);
                ta->isWriterWaiting = false;
            }
            if (!ta->isReadActive &&
                    testId != ServiceIgnoreIncoming &&
                    testId != ServiceDontEcho) {
                unexpectedReaderExit = true;
            }
        }

        if (!unexpectedReaderExit) {
            dataToWrite = buf.data;
            if ( ta->isReadActive ) {
                ta->sent_bufs.push_back(buf);
                buf.data = 0;
                ta->numQueuedBytes += buf.len;
            }
        }

        if (last_write == true) {
            ta->isdone = true;
        }

        err = VPLMutex_Unlock(ta->mutex);
        if (err != VPL_OK) {
            LOG_ERROR("inst %u: VPLMutex_Unlock failed %d", t, err);
            first_err = err;
            goto exit;
        }

        if (unexpectedReaderExit) {
            LOG_ERROR("inst %u: reader exit detected", t);
            err = -1;
            first_err = err;
            goto exit;
        }

        err = TS_EXT::TS_Write(ta->io_handle, dataToWrite, buf.len, error_msg);

        if (testId == ClientCloseWhileServiceWriting) {
            LOG_TRACE ("inst %u testId %d: total bytes previously written "FMTu64, t, testId, totalBytesSent);
            if ( err == TS_ERR_CLOSED || err == TS_ERR_BAD_HANDLE ) {
                // might get either TS_ERR_CLOSED or TS_ERR_BAD_HANDLE if tunnel was closed
                if ( totalBytesSent > EchoMinBytesBeforeClientCloseWhileSvcWriting ) {
                    if (err == TS_ERR_CLOSED) {
                        LOG_ALWAYS("inst %u testId %d TS_Write got expected TS_ERR_CLOSED", t, testId);
                    } else {
                        LOG_ALWAYS("inst %u testId %d TS_Write got expected TS_ERR_BAD_HANDLE when expect tunnel was closed", t, testId);
                    }
                }
                else {
                    LOG_ALWAYS("inst %u testId %d TS_Write got TS_ERR_CLOSED before it was expected", t, testId);
                    first_err = err;
                }
                goto exit;
            }
        }

        if ( err != TS_OK ) {
            if (testId == ServiceCloseAfterWrite && last_write == true) {
                // For this test there is only one set of data written and received/checked
                // and one set that expect write fail because server closed tunnel.
                goto exit;
            }
            if (testId == ServiceCloseWhileClientWritng || testId == ServiceIgnoreIncoming) {
                goto exit;
            }
            LOG_ERROR("inst %u() %d:%s", t, err, error_msg.c_str());
            first_err = err;
            goto exit;
        }

        if (testId == ClientCloseAfterWrite && last_write == true) {
            err = VPLMutex_Lock(ta->mutex);
            if (err != VPL_OK) {
                LOG_ERROR("inst %u: VPLMutex_Lock failed %d", t, err);
                first_err = err;  // will exit loop because this was last write;
            }
            else if (ta->isTunnelClosed) {
                LOG_ERROR("inst %u: ClientCloseAfterWrite expected tunnel to be open", t);
                first_err = err;
            }
            else {
                ta->isTunnelClosed = tunnelWasClosed = true;
                err = TS_EXT::TS_Close(ta->io_handle, error_msg);
                if ( err != TS_OK ) {
                    LOG_ERROR("inst %u: ClientCloseAfterWrite TS_Close() %d:%s", t, err, error_msg.c_str());
                    first_err = err;
                } else {
                    LOG_ALWAYS("inst %u: ClientCloseAfterWrite TS_Close success", t);
                }
            }
            err = VPLMutex_Unlock(ta->mutex);
            if (err != VPL_OK) {
                LOG_ERROR("inst %u: VPLMutex_Unlock failed %d", t, err);
                if (!first_err) { first_err = err; }
            }
        }

        totalBytesSent += buf.len;
        totalBytesRemaining = totalBytes - totalBytesSent;
        if ( totalBytesSent >= logWhen_totalBytesSent ) {
            LOG_ALWAYS ("inst %u: wrote "FMTu64" of "FMTu64" bytes, "FMTu64" remaining.",
                        t, totalBytesSent, totalBytes, totalBytesRemaining);
            logWhen_totalBytesSent += logEvery_bytesSent;
        }

        if (testId == ServiceCloseAfterWrite && last_write == false) {
            // Sleep to allow service to exit after server writes, before client writes again
            // For this test there is only one set of data written and received/checked
            // and one set that expect write fail because server closed tunnel.
            VPLThread_Sleep(VPLTime_FromSec(20));
            LOG_ALWAYS("Before 2nd write for ServiceCloseAfterWrite");
        }
        else {
            if ( test.client_write_delay ) {
                VPLThread_Sleep(VPLTime_FromMillisec(test.client_write_delay));
            }
        }
    }

exit:
    totalBytesRemaining = totalBytes - totalBytesSent;
    numChunksWritten = numChunks - chunksToWrite;
    if (totalBytesRemaining) {
        LOG_ALWAYS ("inst %u: Exiting.  Wrote "FMTu_size_t" bytes %u times.  Wrote "FMTu64" of "FMTu64" bytes, "FMTu64" bytes not written.",
                    t, chunkSize, numChunksWritten, totalBytesSent, totalBytes, totalBytesRemaining);
    } else  {
        LOG_ALWAYS ("inst %u: Exiting.  Wrote "FMTu_size_t" bytes %u times.  Wrote "FMTu64" of "FMTu64" bytes.",
                    t, chunkSize, numChunksWritten, totalBytesSent, totalBytes);
    }
    if (err == TS_ERR_CLOSED || first_err == TS_ERR_CLOSED) {
        tunnelWasClosed = true;
    }
    if (testId == ServiceCloseAfterWrite ||
        testId == ClientCloseWhileServiceWriting ||
        testId == ServiceCloseWhileClientWritng ||
        testId == ServiceDoNothing ||
       (testId == ServiceIgnoreIncoming && numChunksWritten > 3)) {
        if ( err != TS_ERR_CLOSED && err != TS_ERR_BAD_HANDLE ) {
            LOG_ERROR("inst %u testId %d  TS_Write expected TS_ERR_CLOSED or TS_ERR_BAD_HANDLE got %d:%s", t, testId, err, error_msg.c_str());
            if (!err) {
                err = -1;
            }
            if (!first_err) { first_err = err; }
        }
    }
    err = VPLMutex_Lock(ta->mutex);
    if (err != VPL_OK) {
        LOG_ERROR("inst %u: VPLMutex_Lock failed %d", t, err);
        if (!first_err) { first_err = err; }
    }
    
    ta->isdone = true;
    if (tunnelWasClosed) {
        ta->isTunnelClosed = true;
    }
    if (first_err && !ta->isTunnelClosed) {
        // Exiting because of error and tunnel wasn't closed.
        // Close it so reader won't be stuck on read for a long time.
        ta->isTunnelClosed = true;
        err = TS_EXT::TS_Close(ta->io_handle, error_msg);
        if ( err != TS_OK ) {
            LOG_ERROR("inst %u: TS_Close() on error exit %d:%s", t, err, error_msg.c_str());
            if (!first_err) { first_err = err; }
        } else {
            LOG_ALWAYS("inst %u: TS_Close on error exit success", t);
        }
    }

    err = VPLMutex_Unlock(ta->mutex);
    if (err != VPL_OK) {
        LOG_ERROR("inst %u: VPLMutex_Unlock failed %d", t, err);
        if (!first_err) { first_err = err; }
    }

    LOG_ALWAYS("inst %u result: %d", t, first_err);
    return (VPLThread_return_t)first_err;
}


// Thread for client read
static VPLThread_return_t ts_read (VPLThread_arg_t arg)
{
    TSError_t err;
    string error_msg;
    EchoClientRWThreadArgs *ta = (EchoClientRWThreadArgs *)arg;
    const TSTestParameters& test = *ta->testParameters;
    s32 testId = test.testId;
    u32 t = ta->clientInstanceId;
    size_t chunkSize =  test.xfer_size;
    u32 numChunks = test.xfer_cnt;
    u32 totalChunks = numChunks;
    u64 totalBytes = totalChunks * chunkSize;
    u64 total_bytes_read = 0;
    u64 total_bytes_compared = 0;
    u64 logWhen_totalBytesRead = 0;
    u64 logEvery_bytesRead = 0;  // set based on logEnableLevel
    size_t poppedQueBytes = 0;
    bool closed_by_client_while_service_writing = false;
    bool expectToReceiveOrMoreToCheck = true;
    bool expect_ts_err_closed = false;
    bool expect_tunnel_closed_by_server = false;
    bool tunnelWasClosed = false;
    DataToEcho sent;
    const size_t  readBufSize = 32*1024;
    char *buf = new char[readBufSize];


    ta->isReadActive = true;

    LOG_INFO ("inst %u testId %d: Starting thread", t, testId);

    switch (test.logEnableLevel) {
        // There are no logs at all when not actively echoing.
        // Only large data tranfers (over 6 GB) will get more than 1 log
        // based on logEvery_bytesRead at the default logEnableLevel.
        // Logs per time estimates are for echoing continuously on ubuntu vm (ymmv).
        case LOG_LEVEL_TRACE:
        case LOG_LEVEL_DEBUG:
            logEvery_bytesRead = 10*1024*1024; // < 704 logs per hour (< 12 per minute)
            break;
        case LOG_LEVEL_INFO:
            // logEvery_bytesRead = 1024*1024*1024; // < 7 logs per hour (< 1.2 logs per 10 min)
            logEvery_bytesRead = 7334858760ULL; // about 1 log per hour (6.8311195448 GB)
            break;
        case LOG_LEVEL_WARN:
            logEvery_bytesRead = 50*1024*1024*1024ULL; // about 3 logs per day
            break;
        case LOG_LEVEL_ERROR:
        case LOG_LEVEL_CRITICAL:
        case LOG_LEVEL_ALWAYS:
        default:
            logEvery_bytesRead = 150*1024*1024*1024ULL; // about one log per day
            break;
    }

    while (expectToReceiveOrMoreToCheck) {
        size_t read_len;
        size_t read_len_to_compare;
        size_t read_len_compared;
        size_t sent_len_to_compare;
        size_t buf_len = readBufSize;

        if (testId == ServiceCloseAfterWrite) {
            // sleep to allow service to exit after server writes, before client reads
            // For this test there is only one set of data written and received/checked
            // and one set that expect write fail because server closed tunnel.
            VPLThread_Sleep(VPLTime_FromSec(10));
            LOG_ALWAYS("Before read for ServiceCloseAfterWrite");
        }
 
        err = TS_EXT::TS_Read(ta->io_handle, buf, buf_len, error_msg);
        read_len = buf_len;

        LOG_TRACE("inst %u testId %d TS_Read returned buf_len "FMTu_size_t"  %d:%s  ",
                  t, testId, read_len, err, error_msg.c_str());

        if (testId == ServiceCloseAfterWrite) {
            if (expect_tunnel_closed_by_server) {
                expect_ts_err_closed = true;
            }
            else {
                expect_tunnel_closed_by_server = true;
            }
        }
 
        if (testId == ClientCloseWhileServiceWriting) {
            // after reading some bytes, close tunnel while service still writing
            if (closed_by_client_while_service_writing) {
                if (err || (!err && read_len == 0)) {
                    expect_ts_err_closed = true;
                }
            }
            else if ( total_bytes_read > EchoMinBytesBeforeClientCloseWhileSvcWriting ) {
                VPLMutex_Lock(ta->mutex);
                if (ta->isTunnelClosed) {
                    LOG_ERROR("inst %u: ClientCloseWhileServiceWriting expected open tunnel", t);
                    err = -1;
                }
                else {
                    ta->isTunnelClosed = tunnelWasClosed = true;
                    err = TS_EXT::TS_Close(ta->io_handle, error_msg);
                    if ( err != TS_OK ) {
                        LOG_ERROR("inst %u: ClientCloseWhileServiceWriting TS_Close() %d:%s", t, err, error_msg.c_str());
                    } else {
                        LOG_ALWAYS("inst %u: ClientCloseWhileServiceWriting TS_Close success", t);
                    }
                    closed_by_client_while_service_writing = true;
                }
                VPLMutex_Unlock(ta->mutex);
                if (err != VPL_OK) {
                    goto exit;
                }
            }
        }

        if (testId == ServiceCloseWhileClientWritng && (err || (!err && read_len == 0))) {
            expect_ts_err_closed = true;
        }

        if (testId == ServiceDoNothing) {
            expect_ts_err_closed = true;
        }
 
        if (testId == ServiceIgnoreIncoming ) {
            // Expect only 3 chunks echoed for ServiceIgnoreIncoming
            u64 expectedTotalBytes = 3 * chunkSize;
            if (total_bytes_read > expectedTotalBytes) {
                LOG_ERROR("inst %u testId %d:  Expected to received "FMTu64" bytes, got "FMTu64,
                            t, testId, expectedTotalBytes, total_bytes_read);
                if (!err) {
                    err = -1;
                }
            }
            else if (total_bytes_read == expectedTotalBytes) {
                expect_ts_err_closed = true;
            }
        }
 
        if (testId == ClientCloseAfterWrite || expect_ts_err_closed) {
            if ( err == TS_ERR_BAD_HANDLE ) {
                // might get either TS_ERR_CLOSED or TS_ERR_BAD_HANDLE if tunnel was closed
                LOG_ALWAYS("inst %u testId %d: TS_Read got expected TS_ERR_BAD_HANDLE when expect tunnel was closed", t, testId);
                tunnelWasClosed = true;
                err = TS_OK;
            } else if ( err == TS_ERR_CLOSED ) {
                LOG_ALWAYS("inst %u testId %d: TS_Read got expected TS_ERR_CLOSED", t, testId);
                tunnelWasClosed = true;
                err = TS_OK;
            } else if ( !err && read_len == 0 ) {
                LOG_ALWAYS("inst %u testId %d: TS_Read got expected TS_OK && read_len 0", t, testId);
                tunnelWasClosed = true;
            } else {
                LOG_ERROR("inst %u testId %d:  TS_Read expected TS_ERR_CLOSED or TS_ERR_BAD_HANDLE got %d:%s", t, testId, err, error_msg.c_str());
                if (!err) {
                    err = -1;
                }
            }
            goto exit;
        }
        if ( err != TS_OK ) {
            LOG_ERROR("inst %u %d:%s", t, err, error_msg.c_str());
            goto exit;
        }

        read_len_compared = 0;
        read_len_to_compare = read_len;        
        total_bytes_read += read_len;

        while (read_len_compared < read_len) {
            err = VPLMutex_Lock(ta->mutex);
            if (err != VPL_OK) {
                LOG_ERROR("inst %u: VPLMutex_Lock failed %d", t, err);
            }

            if ( !sent.data && ta->sent_bufs.size() == 0 ) {
                LOG_ALWAYS("inst %u: ERROR! Client received data, but no data to compare", t);
                VPLMutex_Unlock(ta->mutex);
                err = -1;
                goto exit; 
            }
            if (!sent.data) {
                ta->numQueuedBytes -= poppedQueBytes;
                poppedQueBytes = 0;
                sent = ta->sent_bufs.front();
                ta->sent_bufs.front().data = 0;  // so pop won't delete the data
                ta->sent_bufs.pop_front();

                // if less then n allocated bytes in sent queue and writer is waiting, signal
                if (ta->isWriterWaiting  && ta->numQueuedBytes < sentQueueContLimit) {
                    err = VPLCond_Signal(ta->cond);
                    if (err != VPL_OK) {
                        LOG_ERROR("inst %u: VPLCond_Signal failed %d", t, err);
                        goto exit; 
                    }
                }
            }

            LOG_TRACE("inst %u: number of sent buffers remaining to receive and compare: "FMTu_size_t"", t, ta->sent_bufs.size() + 1);

            err = VPLMutex_Unlock(ta->mutex);
            if (err != VPL_OK) {
                LOG_ERROR("inst %u: VPLMutex_Unlock failed %d", t, err);
            }

            sent_len_to_compare = sent.len - sent.lenCompared;

            if (read_len_to_compare < sent_len_to_compare) {
                err = memcmp (buf+read_len_compared, &sent.data[sent.lenCompared], read_len_to_compare);
                if (err != 0) {
                    LOG_ERROR("!!!!!! inst %u: Data Read by Client does not match the Data Written", t);
                    goto exit;
                }
                read_len_compared += read_len_to_compare;
                total_bytes_compared += read_len_to_compare;
                sent.lenCompared += read_len_to_compare;
                read_len_to_compare = 0;
            } else {
                // remaining read bytes to compare >= sent bytes block to compare
                err = memcmp (buf+read_len_compared, &sent.data[sent.lenCompared], sent_len_to_compare);
                if (err != 0) {
                    LOG_ERROR("!!!!!! inst %u: Data Read by Client does not match the Data Written", t);
                    goto exit;
                }
                read_len_compared += sent_len_to_compare;
                total_bytes_compared += sent_len_to_compare;
                delete[] sent.data;
                sent.data = 0;
                poppedQueBytes = sent.len;
                read_len_to_compare -= sent_len_to_compare; // either 0 or more of read data to compare
            }
        }
        {
            u64 numQueuedBytes;
            u32 numQueuedBuffers;
            err = VPLMutex_Lock(ta->mutex);
            if (err != VPL_OK) {
                LOG_ERROR("inst %u: VPLMutex_Lock failed %d", t, err);
            }

            numQueuedBuffers = (u32) ta->sent_bufs.size();

            if (!sent.data) {
                ta->numQueuedBytes -= poppedQueBytes;
                poppedQueBytes = 0;
                if (numQueuedBuffers) {
                    sent = ta->sent_bufs.front();
                    ta->sent_bufs.front().data = 0;  // so pop won't delete the data
                    ta->sent_bufs.pop_front();
                }

                // if less then n allocated bytes in sent queue and writer is waiting, signal
                if (ta->isWriterWaiting  && ta->numQueuedBytes < sentQueueContLimit) {
                    err = VPLCond_Signal(ta->cond);
                    if (err != VPL_OK) {
                        LOG_ERROR("inst %u: VPLCond_Signal failed %d", t, err);
                        goto exit; 
                    }
                }
            }

            numQueuedBytes = ta->numQueuedBytes;

            LOG_TRACE("inst %u: %u buffers ("FMTu64" bytes) queued",
                       t, numQueuedBuffers, numQueuedBytes);
            LOG_TRACE("inst %u: is done queing data to check ? == %d", t, ta->isdone);
            if ((ta->isdone) && ( !sent.data && numQueuedBuffers == 0)) {
                LOG_INFO("inst %u: Finishing because writer is done and there is no more to check.", t);
                expectToReceiveOrMoreToCheck = false;
            }
            err = VPLMutex_Unlock(ta->mutex);
            if (err != VPL_OK) {
                LOG_ERROR("inst %u: VPLMutex_Unlock failed %d", t, err);
            }
            if ( total_bytes_read >= logWhen_totalBytesRead) {
                LOG_ALWAYS("inst %u: Received "FMTu64" of "FMTu64" total bytes, "FMTu64" matched"
                           ", %u buffers ("FMTu64" bytes) queued",
                           t, total_bytes_read, totalBytes, total_bytes_compared,
                           numQueuedBuffers, numQueuedBytes);
                logWhen_totalBytesRead += logEvery_bytesRead;
            }
        }
    }

exit:
    {
        int rv;

        rv = VPLMutex_Lock(ta->mutex);
        if (rv != VPL_OK) {
            LOG_ERROR("inst %u: VPLMutex_Lock failed %d", t, rv);
            if (!err) {err = rv;}
        }

        ta->isReadActive = false;
        ta->numQueuedBytes -= poppedQueBytes;
        if (sent.data) {
            delete[] sent.data;
            sent.data = 0;
            ta->numQueuedBytes -= sent.len;
        }

        if (err == TS_ERR_CLOSED || tunnelWasClosed) {
            ta->isTunnelClosed = true;
        }

        if (err && !ta->isTunnelClosed) {
            // Exiting because of error and tunnel wasn't closed.
            // Close it so writer won't keep writing.
            ta->isTunnelClosed = true;
            rv = TS_EXT::TS_Close(ta->io_handle, error_msg);
            if ( rv != TS_OK ) {
                LOG_ERROR("inst %u: TS_Close() on error exit %d:%s", t, rv, error_msg.c_str());
            } else {
                LOG_ALWAYS("inst %u: TS_Close on error exit success", t);
            }
        }

        rv = VPLCond_Signal(ta->cond);
        if (rv != VPL_OK) {
            LOG_ERROR("inst %u: VPLCond_Signal failed %d", t, rv);
            if (!err) {err = rv;}
        }

        rv = VPLMutex_Unlock(ta->mutex);
        if (rv != VPL_OK) {
            LOG_ERROR("inst %u: VPLMutex_Unlock failed %d", t, rv);
            if (!err) {err = rv;}
        }
    }

    delete[] buf;

    LOG_ALWAYS("inst %u: Exiting.  Received "FMTu64" of "FMTu64" total bytes, "FMTu64" matched",
               t, total_bytes_read, totalBytes, total_bytes_compared);
    LOG_ALWAYS("inst %u result: %d", t, err);
    return (VPLThread_return_t)err;
}




// Thread for echo service virtual tunnel client instance
static VPLThread_return_t echoClient (VPLThread_arg_t targ)
{
    TSError_t err;
    TSError_t first_err = TS_OK;
    string error_msg;
    VPLThread_t ts_write_thread;
    VPLThread_t ts_read_thread;
    VPLThread_return_t thread_rv = 0;
    VPLMutex_t thread_state_mutex;
    VPLCond_t thread_state_cond;
    EchoClientRWThreadArgs args;
    EchoClientThreadArgs *targs = (EchoClientThreadArgs *)targ;
    const TSTestParameters& test = *targs->testParameters;
    u32 t = targs->clientInstanceId;
    TSOpenParms_t tsOpenParms;


    LOG_ALWAYS("Starting execution of client inst %u", t);


    tsOpenParms.user_id = test.tsOpenParms.user_id;
    tsOpenParms.device_id = test.tsOpenParms.device_id;
    tsOpenParms.service_name = test.tsOpenParms.service_name;
    tsOpenParms.credentials = test.tsOpenParms.credentials;
    tsOpenParms.flags = test.tsOpenParms.flags;
    tsOpenParms.timeout = VPLTime_FromMicrosec(test.tsOpenParms.timeout);

    // open tunnel by calling TS_Open
    err = TS_EXT::TS_Open(tsOpenParms, args.io_handle, error_msg);
    if ( err != TS_OK ) {
        LOG_ERROR("inst %u: TS_Open() %d:%s", t, err, error_msg.c_str());
        goto exit1;
    }
    args.isTunnelClosed = false;

    // Init Cond
    err = VPLCond_Init(&thread_state_cond);
    if (err != VPL_OK) {
        LOG_ERROR("Client inst %u: VPLCond_Init() %d", t, err);
        first_err = err;
        goto exit2;
    }

    // Init Mutex
    err = VPLMutex_Init(&thread_state_mutex);
    if (err != VPL_OK) {
        LOG_ERROR("Client inst %u: VPLMutex_Init() %d", t, err);
        first_err = err;
        goto exit3;
    }

    // initialize shared fields for client read and client write threads
    args.testParameters = &test;
    args.mutex = &thread_state_mutex;
    args.cond = &thread_state_cond;
    args.numQueuedBytes = 0;
    args.clientInstanceId = t;
    args.isdone = false;
    args.isReadActive = false;
    args.isWriterWaiting = false;

    // start thread for client side write
    {
        VPLThread_fn_t startRoutine = ts_write;
        int rv = 0;
 
        rv = VPLThread_Create(&ts_write_thread, startRoutine, &args, NULL, "TS_Write_thread");
        if (rv != 0) {
            LOG_ERROR("inst %u: Error creating thread for write", t);
            first_err = rv;
            goto exit4;
        }
        LOG_ALWAYS("inst %u: Created thread for write", t);

    }

    if ( test.testId == ServiceDontEcho ) {
        LOG_ALWAYS("inst %u: ts_read_thread not started because ServiceDontEcho", t);
    }
    else {
        // start thread for client side read
        {
            VPLThread_fn_t startRoutine = ts_read;
            int rv = 0;

            rv = VPLThread_Create(&ts_read_thread, startRoutine, &args, NULL, "TS_Read_thread");
            if (rv != 0) {
                LOG_ERROR("inst %u: Error creating thread for read", t);
                first_err = rv;
                goto exit;
            }
            LOG_ALWAYS("inst %u: Created thread for read", t);

        }

        // wait for client side read thread to finish
        err = VPLThread_Join(&ts_read_thread, &thread_rv);
        if (err != TS_OK) {
            LOG_ERROR("inst %u: VPLThread_Join ts_read_thread returned %d", t, err);
            first_err = err;
        } 
        else if (thread_rv) {
            LOG_ERROR("inst %u: ts_read_thread returned %d", t, (int) thread_rv);
            first_err = (int) thread_rv;
        }
        else {
            LOG_ALWAYS("inst %u: ts_read_thread finished successfully", t);
        }
    }

exit:
    // wait for client side write thread to finish
    err = VPLThread_Join(&ts_write_thread, &thread_rv);
    if (err != TS_OK) {
        LOG_ERROR("inst %u: VPLThread_Join ts_write_thread returned %d", t, err);
        if (!first_err) first_err = err;
    }
    else if (thread_rv) {
        LOG_ERROR("inst %u: ts_write_thread returned %d", t, (int) thread_rv);
        if (!first_err) first_err =  (int) thread_rv;
    }
    else {
        LOG_ALWAYS("inst %u: ts_write_thread finished successfully", t);
    }

exit4:
    // Destroy Mutex
    err = VPLMutex_Destroy(&thread_state_mutex);
    if (err != VPL_OK) {
        LOG_ERROR("inst %u: Client: VPLMutex_Destroy() %d", t, err);
        if (!first_err) first_err = err;
    }

exit3:
    // Destroy Cond
    err = VPLCond_Destroy(&thread_state_cond);
    if (err != VPL_OK) {
        LOG_ERROR("inst %u: Client: VPLMutex_Destroy() %d", t, err);
        if (!first_err) first_err = err;
    }

exit2:
    // close tunnel by calling TS_Close
    err = TS_EXT::TS_Close(args.io_handle, error_msg);
    if (args.isTunnelClosed) {
        if (err == TS_ERR_BAD_HANDLE) {
            LOG_ALWAYS("inst %u: TS_Close() when already closed got expected TS_ERR_BAD_HANDLE", t);
            err = TS_OK;
        } else if ( err != TS_OK ) {
            LOG_ERROR("inst %u: TS_Close() when already closed %d:%s", t, err, error_msg.c_str());
            if (!first_err) {
                first_err = err;
            }
        } else {
            LOG_ALWAYS("inst %u: TS_Close() when already closed got TS_OK", t);
        }
    }
    else if ( err != TS_OK ) {
        LOG_ERROR("inst %u: TS_Close() %d:%s", t, err, error_msg.c_str());
    } else {
        LOG_ALWAYS("inst %u: TS_Close success", t);
    }

exit1:
    if (first_err) {
        err = first_err;
    }

    return (VPLThread_return_t)err;
}


int runTsTest(const TSTestParameters& test, TSTestResult& result)
{
    int rv = VPL_OK;
    TSError_t err = TS_OK;
    TSError_t first_err = TS_OK;
    string error_msg;
    VPLThread_return_t thread_rv = 0;
    VPLThread_t echoClientThread[maxClients];
    bool thread_created[maxClients];
    u32 i;

    err = TS_EXT::TS_Init(error_msg);
    if ( err != TS_OK ) {
        LOG_ERROR("TS_Init() %d:%s", err, error_msg.c_str());
        goto exit;
    }

    setDebugLevel(test.logEnableLevel);
    LOG_ALWAYS("setDebugLevel(%d)", test.logEnableLevel);

    for (u32 testIteration = 1; testIteration <= test.nTestIterations;  ++testIteration) {

        for (i = 0;  i < test.nClients;  ++i) {
            thread_created[i] = false;
        }

        for (i = 0;  i < test.nClients;  ++i) {

            static EchoClientThreadArgs args[maxClients];
            args[i].clientInstanceId = i+1;
            args[i].testParameters = &test;

            // start thread for an echo service virtual tunnel client instance

            VPLThread_fn_t startRoutine = echoClient;
            int rv = 0;
     
            rv = VPLThread_Create(&echoClientThread[i], startRoutine,
                                   &args[i], NULL, "echoClientThread");
            if (rv != 0) {
                LOG_ERROR("Error %d creating thread for echo service virtual tunnel client instance %u", err, i+1);
                first_err = rv;
                break;
            }
            LOG_ALWAYS("Created thread for echo service virtual tunnel client inst %u", i+1);
            thread_created[i] = true;
        }

        for (i = 0;  i < test.nClients && thread_created[i];  ++i) {

            // wait for echo service virtual tunnel client instances completion
            err = VPLThread_Join(&echoClientThread[i], &thread_rv);
            if (err != TS_OK) {
                LOG_ERROR("VPLThread_Join echoClientThread %u returned %d", i+1, err);
                if (!first_err) first_err = err;
            }
            else if (thread_rv) {
                LOG_ERROR("echoClientThread %u returned %d", i+1, (int) thread_rv);
                if (!first_err) first_err = (int) thread_rv;
            }
            else {
                LOG_ALWAYS("echoClientThread %u finished", i+1);
            }
            err = TS_OK;
        }

        if (test.testId == ClientCloseAfterWrite) {
            // wait for service to finish up
            VPLThread_Sleep(VPLTime_FromSec(10));
        }

        {
            std::ostringstream test_iteration;
            test_iteration << "Iteration " << testIteration ;
            if (first_err) {
                LOG_ERROR("ts test iteration %s failed: %d", test_iteration.str().c_str(), first_err);
                break;
            } else {
                LOG_ALWAYS("ts test iteration %s passed.", test_iteration.str().c_str());
            }
        }
    }

    // Shutdown the client library
    TS_EXT::TS_Shutdown();

exit:
    if (first_err) {
        err = first_err;
    }
    else if (rv) {
        err = rv;
    }

    result.return_value = err;

    setDebugLevel(LOG_LEVEL_ERROR);

    return err;
}




