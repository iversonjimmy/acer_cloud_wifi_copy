/*
 *  Copyright 2013 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#include "Packet.hpp"
#include "Vtunnel.hpp"
#include "Pool.hpp"

#include "vplex_math.h"

using namespace std;
using namespace Ts2;

static const size_t threadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;

//
// Vtunnel constructor used by TS_Open (client side)
// start sequence number is randomly generated here
//
Ts2::Transport::Vtunnel::Vtunnel(const ts_udi_t& src_udi, const ts_udi_t& dst_udi, const string& svcName, LocalInfo *localInfo) : 
    state(VT_STATE_NULL),
    activeClients(0),
    networkFrontEnd(NULL),
    sendWin(NULL),
    recvWin(NULL),
    src_udi(src_udi), 
    dst_udi(dst_udi),
    dstStartSeqNum(0),
    svcName(svcName),
    lcl_vt_id(0),
    rmt_vt_id(0),
    isWriterStarted(false),
    isWriterStop(false),
    isRetryThreadStarted(false),
    isRetryStop(false),
    localInfo(localInfo)
{
    VPLMutex_Init(&mutex);
    VPLCond_Init(&open_cond);
    VPLCond_Init(&close_cond);
    VPLCond_Init(&clients_cond);

    sendWin = new (std::nothrow) sendWindow(localInfo->GetMaxSegmentSize(), localInfo->GetMaxWindowSize(), localInfo->GetRetransmitInterval());
    recvWin = new (std::nothrow) recvWindow(localInfo->GetMaxWindowSize());
}

//
// Vtunnel constructor for the server side (remote VTID is known)
//
Ts2::Transport::Vtunnel::Vtunnel(const ts_udi_t& src_udi, const ts_udi_t& dst_udi, u64 dstStartSeqNum, u32 dst_vt_id, const string& svcName, LocalInfo *localInfo) : 
    state(VT_STATE_HALF_OPEN), 
    activeClients(0),
    networkFrontEnd(NULL),
    sendWin(NULL),
    recvWin(NULL),
    src_udi(src_udi), 
    dst_udi(dst_udi), 
    dstStartSeqNum(dstStartSeqNum),
    svcName(svcName),
    lcl_vt_id(0),
    rmt_vt_id(dst_vt_id),
    isWriterStarted(false),
    isWriterStop(false),
    isRetryThreadStarted(false),
    isRetryStop(false),
    localInfo(localInfo)
{
    VPLMutex_Init(&mutex);
    VPLCond_Init(&open_cond);
    VPLCond_Init(&close_cond);
    VPLCond_Init(&clients_cond);

    sendWin = new (std::nothrow) sendWindow(localInfo->GetMaxSegmentSize(), localInfo->GetMaxWindowSize(), localInfo->GetRetransmitInterval());
    recvWin = new (std::nothrow) recvWindow(localInfo->GetMaxWindowSize());
}

Ts2::Transport::Vtunnel::~Vtunnel()
{
    stopWorkerThreads();

    delete sendWin;
    delete recvWin;

    VPLCond_Destroy(&open_cond);
    VPLCond_Destroy(&close_cond);
    VPLCond_Destroy(&clients_cond);
    VPLMutex_Destroy(&mutex);

    LOG_DEBUG("Vtunnel[%p]: Destroyed", this);
}

void Ts2::Transport::Vtunnel::SetVtId(u32 newVtId)
{
    // This is a word in memory, so the mutex would do nothing here
    lcl_vt_id = newVtId;
}

u32 Ts2::Transport::Vtunnel::GetVtId()
{
    return lcl_vt_id;
}

u32 Ts2::Transport::Vtunnel::GetDstVtId()
{
    return rmt_vt_id;
}

u64 Ts2::Transport::Vtunnel::GetDstDeviceId()
{
    return dst_udi.deviceId;
}

u32 Ts2::Transport::Vtunnel::GetDstInstanceId()
{
    return dst_udi.instanceId;
}

u64 Ts2::Transport::Vtunnel::GetDstStartSeqNum()
{
    return dstStartSeqNum;
}

bool Ts2::Transport::Vtunnel::IsConnectingDevice(u64 deviceId)
{
    return (src_udi.deviceId == deviceId) || (dst_udi.deviceId == deviceId);
}

void Ts2::Transport::Vtunnel::SetFrontEnd(Ts2::Network::FrontEnd *frontEnd)
{
    //
    // Do not allow this value to be reassigned once it is set.  This is probably
    // paranoid, but the lower level code sometimes dereferences this function pointer
    // without holding any locks.
    //
    MutexAutoLock lock(&mutex);
    if (networkFrontEnd == NULL) {
        networkFrontEnd = frontEnd;
    } else {
        LOG_ERROR("Vtunnel[%p]: Attempt to reset NFE function pointer: lcl vt "FMTu32, this, lcl_vt_id);
    }
}

int Ts2::Transport::Vtunnel::AddRefCount(int delta)
{
    // Actually protected by mutex of owning Pool
    MutexAutoLock lock(&mutex);
    activeClients += delta;
    assert(activeClients >= 0);
    if (activeClients == 0 && state == VT_STATE_CLOSING) {
        VPLCond_Broadcast(&clients_cond);
    }
    return activeClients;
}

int Ts2::Transport::Vtunnel::GetRefCount()
{
    return activeClients;
}

int Ts2::Transport::Vtunnel::Open(string& err_msg)
{
    int err = 0;

    err_msg.clear();
    {
        MutexAutoLock lock(&mutex);
        // State check and transition.
        if (state == VT_STATE_CLOSED) {
            LOG_ERROR("Vtunnel[%p]: Tunnel has been forced closed due to unauthorized device", this);
            err = TS_ERR_INVALID_DEVICE;
            goto end;
        }
        else if (state != VT_STATE_NULL) {
            LOG_ERROR("Vtunnel[%p]: Wrong tunnel state %d", this, state);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        state = VT_STATE_OPENING;
        Ts2::PacketSyn* synPkt = new (std::nothrow) Ts2::PacketSyn(
                                                                   src_udi.deviceId, src_udi.instanceId, lcl_vt_id, 
                                                                   dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id, 
                                                                   sendWin->cur_seq, 
                                                                   recvWin->cur_ack, 
                                                                   svcName);
        if (synPkt == NULL) {
            err = TS_ERR_NO_MEM;
            err_msg = "Memory allocation failed";
            goto end;
        }

#ifdef TS2_PKT_RETRY_ENABLE
        // Spawn retry thread for later packet retransmission
        if (!isRetryThreadStarted) {
            LOG_DEBUG("Spawn retry thread for reliable transmission...");
            err = Util_SpawnThread(retryThread, this, threadStackSize, /*isJoinable*/VPL_TRUE, &retryHandle);
            if (err != VPL_OK) {
                LOG_ERROR("Spawn retry thread fails: %d", err);
                err = TS_ERR_INTERNAL;
                delete synPkt;
                goto end;
            }
            isRetryThreadStarted = true;
        }
        // insert to segment list
        {
            // Note: this nests the segs_mutex within the buffers_mutex.
            // Make sure this is the only order in which these are nested.
            MutexAutoLock lock(&sendWin->segs_mutex);
    
            send_segment *seg = new (std::nothrow) send_segment;
            if (seg == NULL) {
                err = TS_ERR_NO_MEM;
                LOG_ERROR("Allocation failure: %d", err);
                err_msg = "Memory allocation failed";
                goto end;
            }
            // maintain information of SYN packet
            // so we can re-contruct SYN packet during retransmission
            seg->pktType = TS_PKT_SYN;
            seg->seqNum = synPkt->GetSeqNum();
            seg->ackNum = synPkt->GetAckNum();
            seg->svcName = svcName;
            seg->data = NULL;
            seg->dataSize = 0;
            seg->sendTimestamp = VPLTime_GetTimeStamp();
            seg->retryRound = 1;  // default retry round is 1 (for retry interval exponential back-off)
            seg->retryInterval = sendWin->GetCurrentDefaultRetryInterval();  // default retry interval of each segment
            seg->head_seq_index = seg->seqNum;  // for later segment cleanup after SYN ACK received
            err = sendWin->InsertSegments(seg);
            if (err != TS_OK) {
                // clean up segment and synPkt if failed
                LOG_ERROR("Segment insertion failure, %d", err);
                err_msg = "Segment insertion failed";
                delete synPkt;
                seg->data = NULL;
                delete seg;
                goto end;
            }
        }
#endif
        // enqueue packet to FrontEnd
        LOG_DEBUG("Sending SYN packet %p from lcl "FMTu32" to rmt "FMTu32, synPkt, lcl_vt_id, rmt_vt_id);
        sendPacket(synPkt, sendWin->GetCurrentDefaultRetryInterval());
        // The lower layers will free the synPkt once it is transmitted
        synPkt = NULL;

        // Wait for the SYN_ACK response from the server side
        LOG_DEBUG("Vtunnel[%p]: wait for Open to complete for vt "FMTu32, this, lcl_vt_id);
        while (state == VT_STATE_OPENING) {
            // FIXME: should implement cumulative wait time if we loop here
            err = VPLCond_TimedWait(&open_cond, &mutex, localInfo->GetTsOpenTimeout());
            if (err == VPL_ERR_TIMEOUT) {
                LOG_ERROR("Waited %d ms for SYN_ACK, open fails for vt "FMTu32,
                    (int)VPLTime_ToMillisec(localInfo->GetTsOpenTimeout()), lcl_vt_id);
                err = TS_ERR_TIMEOUT;
                goto end;
            }
            if (err) {
                LOG_ERROR("Vtunnel[%p]: CondVar failed: err %d", this, err);
                goto end;
            }
            // If a shutdown is initiated, that will abort any pending open.  Note that this
            // logic is redundant, but gives an explicit message showing this case was hit.
            if (state == VT_STATE_CLOSED) {
                LOG_ERROR("Vtunnel[%p]: Open aborted for vt "FMTu32, this, lcl_vt_id);
                err = TS_ERR_CLOSED;
                goto end;
            }
            if (openQueue.empty()) {
                continue;
            }
            Packet *pkt = openQueue.front();
            openQueue.pop_front();
            if (pkt->GetPktType() != TS_PKT_SYN_ACK) {
                LOG_ERROR("Drop Packet %p: not a SYN_ACK (type %u)", pkt, pkt->GetPktType());
                delete pkt;
                continue;
            }
            {
                // this is a little sloppy in that we should probably grab the sendWin mutex here
                // although no other thread can probably get at the tunnel until state goes to OPEN.
                // Nests sendWindow segs_mutex in overall mutex
                MutexAutoLock segLock(&sendWin->segs_mutex);
                if (pkt->GetAckNum() != sendWin->cur_seq + 1) {
                    LOG_ERROR("Drop Packet %p: not a valid ack number (type "FMTu64", "FMTu64")", pkt, pkt->GetAckNum(), sendWin->cur_seq);
                    delete pkt;
                    continue;
                }
                // Init window size from SYN ACK
                sendWin->SetWindowSize(pkt->GetWindowSize());
                // Init seq & ack number
                sendWin->nextBufferIndex = sendWin->currBufferIndex = sendWin->cur_seq = pkt->GetAckNum();
            }
            {
                // Nests recvWindow segs_mutex in overall mutex
                MutexAutoLock recv_lock(&recvWin->segs_mutex);
                recvWin->cur_ack = pkt->GetSeqNum() + 1;
                recvWin->head_seq = recvWin->cur_ack;
            }
            // Source VTID is the other end of the connection
            rmt_vt_id = pkt->GetSrcVtId();
            if (rmt_vt_id == 0) {
                LOG_ERROR("Drop Packet %p: SYN_ACK with 0 vtid", pkt);
                delete pkt;
                continue;
            }
#ifndef TS2_PKT_RETRY_ENABLE
            // Spawn retry thread for later packet retransmission
            if (!isRetryThreadStarted) {
                LOG_DEBUG("Spawn retry thread for reliable transmission...");
                err = Util_SpawnThread(retryThread, this, threadStackSize, /*isJoinable*/VPL_TRUE, &retryHandle);
                if (err != VPL_OK) {
                    LOG_ERROR("Spawn retry thread fails: %d", err);
                    err = TS_ERR_INTERNAL;
                    delete pkt;
                    break;
                }
                isRetryThreadStarted = true;
            }
#endif
            // Send ACK to SYN_ACK and we are ready to rock and roll
            LOG_INFO("Vtunnel[%p]: got SYN_ACK, go to OPEN state for lcl "FMTu32" rmt "FMTu32, this, lcl_vt_id, rmt_vt_id);
            state = VT_STATE_OPEN;
            // Cleanup redundant packets in openQueue
            openQueue.clear();
            delete pkt;
            Ts2::PacketAck* ackPkt = new (std::nothrow) Ts2::PacketAck(
                                                                       src_udi.deviceId, src_udi.instanceId, lcl_vt_id, 
                                                                       dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id, 
                                                                       sendWin->cur_seq, recvWin->cur_ack, 
                                                                       recvWin->window_capacity - recvWin->recvSegments_size);
            if (ackPkt == NULL) {
                err = TS_ERR_NO_MEM;
                err_msg = "Memory allocation failed";
                goto end;
            }
            LOG_DEBUG("Sending ACK packet %p", ackPkt);
            sendPacket(ackPkt, 0);
            break;
        }
        if (state != VT_STATE_OPEN) {
            err = TS_ERR_NO_CONNECTION;
        }
    }

end:
#ifdef TS2_PKT_RETRY_ENABLE
    // Cancel non-DATA packet in sendSegments & retrySegments
    sendWin->CancelCtrlPktFromSegments();
#endif
    return err;
}

int Ts2::Transport::Vtunnel::WaitForOpen()
{
    int err = 0;

    //
    // Called by the server side after sending the SYN_ACK:  wait for the
    // completion of the three-way handshake
    //
    do {
        MutexAutoLock lock(&mutex);
        //
        // If a shutdown has started, the open will be aborted
        //
        if (state == VT_STATE_CLOSED) {
            LOG_ERROR("Vtunnel[%p]: open aborted for vt "FMTu32, this, lcl_vt_id);
            err = TS_ERR_CLOSED;
            break;
        }
        // There is a race with the ACK packet from the client side and the call to this routine.
        // If state is already VT_STATE_OPEN, then the ACK has already been received.
        if (state != VT_STATE_HALF_OPEN && state != VT_STATE_OPEN) {
            LOG_ERROR("Vtunnel[%p]: Wrong tunnel state %d", this, state);
            err = TS_ERR_WRONG_STATE;
            break;
        }
        while (state == VT_STATE_HALF_OPEN) {
            LOG_DEBUG("Vtunnel[%p]: wait for final ACK of 3 way HS", this);
            err = VPLCond_TimedWait(&open_cond, &mutex, localInfo->GetTsOpenTimeout());
            if (err == VPL_ERR_TIMEOUT) {
                LOG_ERROR("Vtunnel[%p]: wait in HALF_OPEN timed out: err %d", this, err);
                err = TS_ERR_TIMEOUT;
                break;
            }
            if (err) {
                LOG_ERROR("Vtunnel[%p]: condVar wait failed: err %d", this, err);
                err = TS_ERR_INTERNAL;
                break;
            }
        }
        if (state == VT_STATE_OPEN && err == 0) {
#ifndef TS2_PKT_RETRY_ENABLE
            // Spawn retry thread for later packet retransmission
            if (!isRetryThreadStarted) {
                LOG_DEBUG("Vtunnel[%p]: spawn retry thread for reliable transmission", this);
                err = Util_SpawnThread(retryThread, this, threadStackSize, /*isJoinable*/VPL_TRUE, &retryHandle);
                if (err != VPL_OK) {
                    LOG_ERROR("Vtunnel[%p]: spawn retry thread fails: %d", this, err);
                    err = TS_ERR_INTERNAL;
                    break;
                }
                isRetryThreadStarted = true;
            }
#endif
        } else if (state == VT_STATE_CLOSED) {
            LOG_ERROR("Vtunnel[%p]: open aborted for vt "FMTu32, this, lcl_vt_id);
            err = TS_ERR_CLOSED;
            break;
        }
    } while (ALWAYS_FALSE_CONDITIONAL);

#ifdef TS2_PKT_RETRY_ENABLE
    // Cancel non-DATA packets
    sendWin->CancelCtrlPktFromSegments();
#endif

    return err;
}

int Ts2::Transport::Vtunnel::WaitForClose()
{
    int err = 0;

    //
    // Called by the server side when the service thread thinks it is done.
    // Send FIN to client, then wait for either FIN or FIN_ACK from client.
    //
    // There is a race in the case that the server is writing:  the client
    // may recognize the completeness of the transfer and send a FIN before
    // the server gets here.  There may be other cases in which the server
    // FIN is required to notify the client that the transfer is complete.
    //
    do {
        MutexAutoLock lock(&mutex);
        if (state == VT_STATE_CLOSED) {
            LOG_DEBUG("Vtunnel[%p]: vt "FMTu32" already closed", this, lcl_vt_id);
            break;
        }
        if (state != VT_STATE_OPEN && state != VT_STATE_CLOSING) {
            LOG_ERROR("Vtunnel[%p]: Wrong tunnel state %d for vt "FMTu32, this, state, lcl_vt_id);
            err = TS_ERR_WRONG_STATE;
            break;
        }
        if (state == VT_STATE_CLOSING) {
            LOG_DEBUG("Vtunnel[%p]: FIN already received for vt "FMTu32, this, lcl_vt_id);
            closeTunnel(/*sendFin*/false);
            state = VT_STATE_CLOSED;
            // In case Close was also called
            VPLCond_Broadcast(&close_cond);
        } else {
            // Initiate Close: stop IO threads and send FIN
            closeTunnel(/*sendFin*/true);
            LOG_DEBUG("Vtunnel[%p]: wait for FIN or FIN_ACK to close vt "FMTu32, this, lcl_vt_id);
            state = VT_STATE_CLOSE_WAIT;
            while (state == VT_STATE_CLOSE_WAIT) {
                err = VPLCond_TimedWait(&close_cond, &mutex, localInfo->GetTsCloseTimeout());
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_ERROR("Waited %d ms for FIN or FIN_ACK, force close for vt "FMTu32,
                              (int)VPLTime_ToMillisec(localInfo->GetTsCloseTimeout()), lcl_vt_id);
                    err = TS_ERR_TIMEOUT;
                    state = VT_STATE_CLOSED;
                    break;
                }
                if (err) {
                    LOG_ERROR("Vtunnel[%p]: CondVar failed: err %d", this, err);
                    break;
                }
            }
            if (state != VT_STATE_CLOSED) {
                LOG_ERROR("Vtunnel[%p]: unexpected vt state %d", this, state);
            }
            LOG_DEBUG("Vtunnel[%p]: close complete for vt "FMTu32, this, lcl_vt_id);
        }
    } while (ALWAYS_FALSE_CONDITIONAL);

    return err;
}

int Ts2::Transport::Vtunnel::ForceClose()
{
    int err = 0;

    // Currently called by two situations:
    //
    // 1. Called by the Pool::Stop code when shutdown happens.  
    // 2. Called by the Pool::eventHandlerThread for dropping tunnels with 
    // unlinked device.
    // 
    // Either server or client side initiates Close() and causes any threads in the
    // middle of Open(), Read() or Write() operations to be aborted with errors.
    // On client side, it eliminates the ability to open a tunnel before Open() is called.
    // The other difference from normal close is that the timeouts should be
    // shorter than for normal close since it probably means CCD is shutting
    // down altogether and that takes long enough as it is.
    //
    // Send FIN, then wait for either FIN or FIN_ACK from the other end.
    //
    // Note that the waiting for other client threads to release this tunnel
    // happens at the Pool::Stop level, not here.
    //
    do {
        MutexAutoLock lock(&mutex);
        if (state == VT_STATE_CLOSED) {
            LOG_INFO("Vtunnel[%p]: vt "FMTu32" already closed", this, lcl_vt_id);
            break;
        }
        if (state == VT_STATE_NULL) {
            // Open() is not yet called
            // Set state to VT_STATE_CLOSED to make Open() fail
            LOG_INFO("Vtunnel[%p]: abort even before open for vt "FMTu32, this, lcl_vt_id);
            state = VT_STATE_CLOSED;
            break;
        }
        if (state == VT_STATE_OPENING || state == VT_STATE_HALF_OPEN) {
            LOG_INFO("Vtunnel[%p]: abort open in progress for vt "FMTu32, this, lcl_vt_id);
            state = VT_STATE_CLOSED;
            VPLCond_Broadcast(&open_cond);
            break;
        }
        if (state != VT_STATE_OPEN && state != VT_STATE_CLOSING && state != VT_STATE_CLOSE_WAIT) {
            LOG_ERROR("Vtunnel[%p]: Wrong tunnel state %d for vt "FMTu32, this, state, lcl_vt_id);
            err = TS_ERR_WRONG_STATE;
            break;
        }
        LOG_INFO("Vtunnel[%p]: force close vt "FMTu32", state %u", this, lcl_vt_id, state);
        if (state == VT_STATE_CLOSING) {
            LOG_INFO("Vtunnel[%p]: FIN already received for vt "FMTu32, this, lcl_vt_id);
            closeTunnel(/*sendFin*/false);
            state = VT_STATE_CLOSED;
        } else {
            // Initiate Close: stop IO threads and send FIN
            closeTunnel(/*sendFin*/true);
#if 0
            //
            // There is no point in waiting for FIN_ACK at this point. We don't retry
            // FINs (yet), so there is nothing more to be done.
            //
            // TODO: This decision should be reconsidered when we implement retry of control packets.
            //
            LOG_INFO("Vtunnel[%p]: wait for FIN or FIN_ACK to close vt "FMTu32, this, lcl_vt_id);
            state = VT_STATE_CLOSING;
            while (state == VT_STATE_CLOSING) {
                err = VPLCond_TimedWait(&close_cond, &mutex, localInfo->GetFinWaitTimeout());
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_ERROR("Waited %d ms for FIN or FIN_ACK, force close for vt "FMTu32,
                              (int)VPLTime_ToMillisec(localInfo->GetFinWaitTimeout()), lcl_vt_id);
                    err = TS_ERR_TIMEOUT;
                    break;
                }
                if (err) {
                    LOG_ERROR("Vtunnel[%p]: CondVar failed: err %d", this, err);
                    break;
                }
            }
#endif
            state = VT_STATE_CLOSED;
        }
    } while (ALWAYS_FALSE_CONDITIONAL);

    LOG_INFO("Vtunnel[%p]: after force close vt "FMTu32" err %d clients %d", 
              this, lcl_vt_id, err, activeClients);

    return err;
}
       
int Ts2::Transport::Vtunnel::Read(void* buffer, size_t& len, string& err_msg)
{
    int err = 0;

    err_msg.clear();
    do {
        {
            MutexAutoLock lock(&mutex);

            // State check
            if (state != VT_STATE_OPEN && state != VT_STATE_CLOSING) {
                LOG_ERROR("Vtunnel[%p]: Wrong tunnel state %d", this, state);
                err_msg = "Invalid tunnel state";
                err = TS_ERR_WRONG_STATE;
                len = 0;
                break;
            }
            //
            // If in CLOSING state, don't wait for new data, but allow anything
            // already queued to be consumed
            //
            if (state == VT_STATE_CLOSING) {
                MutexAutoLock recv_lock(&recvWin->segs_mutex);
                // The FIN bumps cur_ack by 1
                if ((recvWin->recvSegments.size() == 0) || ((recvWin->cur_ack - recvWin->head_seq) <= 1)) {
                    LOG_DEBUG("Vtunnel[%p]: CLOSING state with empty recvWin", this);
                    err = TS_ERR_CLOSED;
                    err_msg = "Tunnel is already closed";
                    len = 0;
                    break;
                }
            }
        }
        {
            // Read available data from recvWin
            MutexAutoLock recv_lock(&recvWin->segs_mutex);
            while (!recvWin->closeInProgress && ((recvWin->cur_ack - recvWin->head_seq) == 0)) {
                // Wait until data is received or timeout
                LOG_DEBUG("Vtunnel[%p]: wait for data with cur_ack "FMTu64" head_seq "FMTu64" recvSegment_size "FMTu64,
                    this, recvWin->cur_ack, recvWin->head_seq, recvWin->recvSegments_size);
                recvWin->recvThreadCount++;
                err = VPLCond_TimedWait(&recvWin->recv_cond, &recvWin->segs_mutex, localInfo->GetTsReadTimeout());
                recvWin->recvThreadCount--;
                if (err == VPL_ERR_TIMEOUT) {
                    // TODO: recvWin cleanup
                    LOG_ERROR("Cannot read anything after %d ms", (int)VPLTime_ToMillisec(localInfo->GetTsReadTimeout()));
                    err_msg = "Read timed out";
                    err = TS_ERR_TIMEOUT;
                    goto end;
                }
            }
            // Allow available data to be consumed before the close completes.  Note that the
            // FIN bumps cur_ack by 1, so if CIP and diff == 1 there is no real data to read.
            if (recvWin->closeInProgress && ((recvWin->cur_ack - recvWin->head_seq) <= 1)) {
                LOG_WARN("Tunnel closed by another thread (no pending input)");
                err_msg = "Read aborted by close";
                err = TS_ERR_CLOSED;
                goto end;
            }
            u64 checkAvailToRead = recvWin->cur_ack - recvWin->head_seq;
            // Be careful about 32 bit overflow
            if (checkAvailToRead > 1<<30) {
                LOG_ERROR("Vtunnel[%p]: cur_ack "FMTu64", head_seq "FMTu64,
                           this, recvWin->cur_ack, recvWin->head_seq);
                LOG_ERROR("Vtunnel[%p]: too big a gap in seq numbers "FMTu64, this, checkAvailToRead);
                err_msg = "Invalid internal packet sequence numbers";
                err = TS_ERR_INTERNAL;
                goto end;
            }
            size_t availBufferToRead = (size_t)checkAvailToRead;
            size_t bufferToRead = MIN(len, availBufferToRead);
            size_t bytesRead = 0;
            LOG_DEBUG("Vtunnel[%p]: number of recv segs "FMTu_size_t", avail "FMTu_size_t", len "FMTu_size_t, this, recvWin->recvSegments.size(), availBufferToRead, len);
            map<u64, PacketData*>::iterator it = recvWin->recvSegments.begin();
            while (it != recvWin->recvSegments.end()) {
                // Read as much as possible
                if (bufferToRead == 0) {
                    break;
                }
                // Packets in the recvWin may be partially consumed
                size_t pktOffset = 0;
                u32 pktDataLen = it->second->GetDataSize();
                // Be careful about 32 bit overflow (should never happen)
                u64 seqOffset = recvWin->head_seq - it->second->GetSeqNum();
                if (seqOffset > (1<<30)) {
                    LOG_ERROR("Vtunnel[%p]: head_seq "FMTu64", but new pkt_seq "FMTu64,
                               this, recvWin->head_seq, it->second->GetSeqNum());
                    LOG_ERROR("Vtunnel[%p]: too big a gap in seq numbers "FMTu64, this, seqOffset);
                    err_msg = "Invalid internal packet sequence numbers";
                    err = TS_ERR_INTERNAL;
                    break;
                }
                pktOffset = (size_t)seqOffset;
                if (pktOffset > 0) {
                    LOG_DEBUG("Vtunnel[%p]: first "FMTu_size_t" bytes of packet already consumed", this, pktOffset);
                }
                // Consistency check that the head_seq was really within the current packet
                if (pktOffset > pktDataLen) {
                    // If this happens, the data structures are wrong
                    LOG_ERROR("Vtunnel[%p]: calculated offset too large for current packet: "FMTu_size_t" > "FMTu32,
                              this, pktOffset, pktDataLen);
                    // In theory this "Can't Happen(tm)", so fail the call
                    err_msg = "Invalid internal packet sequence numbers";
                    err = TS_ERR_INTERNAL;
                    break;
                }
                pktDataLen -= pktOffset;
                size_t readSize = MIN(bufferToRead, pktDataLen);

                memcpy((u8*)buffer + bytesRead, it->second->GetData() + pktOffset, readSize);
                bytesRead += readSize;
                recvWin->head_seq += readSize;
                bufferToRead -= readSize;
                // If this read fully consumes the rest of the packet, remove it from the window
                if (readSize == pktDataLen) {
                    LOG_DEBUG("Vtunnel[%p]: remove fully consumed segment", this);
                    recvWin->recvSegments_size -= it->second->GetDataSize();
                    // delete the consumed packet
                    delete it->second;
                    map<u64, PacketData*>::iterator it_cur = it++;
                    recvWin->recvSegments.erase(it_cur);
                } else {
                    // If it did not consume the whole packet, that's all for this read().
                    break;
                }
            }
            LOG_DEBUG("Vtunnel[%p]: bytesRead "FMTu_size_t, this, bytesRead);
            len = bytesRead;
        } // Release recvWin->segs_mutex

    } while (ALWAYS_FALSE_CONDITIONAL);

 end:
    LOG_DEBUG("Vtunnel[%p]: Read returns len "FMTu_size_t" err %d", this, len, err);
    return err;
}

int Ts2::Transport::Vtunnel::Write(const void* buffer, size_t len, string& err_msg)
{
    int err = TS_OK;

    err_msg.clear();
    do {
        u64 index = 0;
        {
            MutexAutoLock lock(&mutex);
            // State check
            if (state != VT_STATE_OPEN) {
                if (state == VT_STATE_CLOSED || 
                    state == VT_STATE_CLOSING || 
                    state == VT_STATE_CLOSE_WAIT) {

                    LOG_ERROR("Vtunnel[%p]: tunnel is closed (state %d) vt "FMTu32, this, state, lcl_vt_id);
                    err_msg = "Tunnel is closed";
                    err = TS_ERR_CLOSED;
                    break;
                }
                LOG_ERROR("Vtunnel[%p]: Wrong tunnel state %d for vt "FMTu32, this, state, lcl_vt_id);
                err_msg = "Invalid tunnel state";
                err = TS_ERR_WRONG_STATE;
                break;
            }
            if (len == 0) {
                LOG_WARN("Vtunnel[%p]: send nothing for vt "FMTu32, this, lcl_vt_id);
                break;
            }
        }
        // append to sendWin's buffers
        VPLCond_t complete_cond;
        VPLCond_Init(&complete_cond);
        // 
        // The condvar object is on the stack, but this thread will not exit this routine
        // until the condvar is signalled (either by the write completing or timing out).
        //
        err = sendWin->AppendToBuffers(buffer, len, &complete_cond, index);
        if (err != TS_OK) {
            LOG_ERROR("Add to sending buffers failed");
            err_msg = "Add to sending buffers failed";
            VPLCond_Destroy(&complete_cond);
            break;
        }
        {
            // Don't start the writer thread until we have a buffer to send
            MutexAutoLock lock(&mutex);
            if (!isWriterStarted && !isWriterStop) {
                if (state != VT_STATE_OPEN) {
                    LOG_DEBUG("Close happened in another thread");
                    err_msg = "Write aborted by Close";
                    err = TS_ERR_CLOSED;
                } else {
                    err = Util_SpawnThread(writerThread, this, threadStackSize, /*isJoinable*/VPL_TRUE, &writerHandle);
                    if (err) {
                        LOG_ERROR("Spawn writer thread fails: %d", err);
                        err_msg = "Resource allocation failure";
                    }
                }
                if (err != TS_OK) {
                    MutexAutoLock bufLock(&sendWin->buffers_mutex);
                    sendWin->CancelFromBuffers(index);
                    VPLCond_Destroy(&complete_cond);
                    break;
                }
                LOG_INFO("Vtunnel[%p]: Writer thread spawned vt "FMTu32, this, lcl_vt_id);
                isWriterStarted = true;
            }
        }
        {
            // There is a race with the writer thread and with close/fin
            MutexAutoLock bufLock(&sendWin->buffers_mutex);
            map<u64, send_buffer*>::iterator it_buf = sendWin->sendBuffers.find(index);
            // Check whether the buffer is already sent or not
            if (it_buf != sendWin->sendBuffers.end()) {
                // Check that Close in another thread or a FIN has not cancelled the buffer
                if (it_buf->second->sendAborted) {
                    LOG_WARN("Write aborted for index "FMTu64, index);
                    err_msg = "Write aborted by Close";
                    // XXX: adjust "len" return value to reflect failure?
                    err = TS_ERR_CLOSED;
                    goto cleanup;
                }
                // Wait until buffer is acked or timeout
                // Timeout is determined as follows:
                // 1) fixed connection establishment overhead of 30 seconds.
                // 2) allow transfer speed to drop to as low as 10KB/s.
                size_t timeout = len / (10 * 1024) + 30;
                err = VPLCond_TimedWait(&complete_cond, &sendWin->buffers_mutex, VPLTime_FromSec(timeout));
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_ERROR("Cannot finish writing after "FMTu_size_t" secs", timeout);
                    err_msg = "Write timed out";
                    err = TS_ERR_TIMEOUT;
                    goto cleanup;
                }
            } else {
                LOG_DEBUG("Vtunnel[%p]: writer has already written the buffer "FMTu64" for vt "FMTu32,
                    this, index, lcl_vt_id);
            }
            // Check if the buffer was aborted
            it_buf = sendWin->sendBuffers.find(index);
            if (it_buf != sendWin->sendBuffers.end()) {
                if (it_buf->second->sendAborted) {
                    LOG_WARN("Write aborted for index "FMTu64, index);
                    err_msg = "Write aborted by Close";
                    err = TS_ERR_CLOSED;
                    // XXX: adjust "len" return value to reflect failure?
                    goto cleanup;
                }
            } else {
                // In the successful case, the entry has already been deleted by the lower level
            }

        cleanup:
            // 
            // Before returning from Write(), we need to guarantee there
            // are no queued data packets waiting to be sent at the Network layer
            // which reference the caller's buffer.
            //
            LOG_DEBUG("Cancel pkts for "FMTu32, lcl_vt_id);
            if (err != TS_OK) {
                // Cancel segments so retryThread won't enqueue them again
                // It should happen all the time before cancelling packets at the Network layer
                sendWin->CancelFromBuffers(index);
            }
            networkFrontEnd->CancelDataPackets(dst_udi.deviceId, dst_udi.instanceId, lcl_vt_id);
        }
        VPLCond_Destroy(&complete_cond);
    } while (ALWAYS_FALSE_CONDITIONAL);

    LOG_DEBUG("Vtunnel[%p]: Write returns len "FMTu_size_t" err %d vt "FMTu32, this, len, err, lcl_vt_id);
    return err;
}

int Ts2::Transport::Vtunnel::HandleSyn(Packet* pkt, s32 error)
{
    int err = 0;
    Ts2::PacketSynAck* synAckPkt = NULL;

    // At present this is only called by the server side

    do {
        MutexAutoLock lock(&mutex);

        if (pkt->GetPktType() != Ts2::TS_PKT_SYN) {
            LOG_ERROR("Drop Packet %p: not a SYN (type %u)", pkt, pkt->GetPktType());
            err = TS_ERR_INVALID;
            break;
        }
#ifdef TS2_PKT_RETRY_ENABLE
        // Source VTID is the other end of the connection
        rmt_vt_id = pkt->GetSrcVtId();
        if (rmt_vt_id == 0) {
            LOG_ERROR("Drop Packet %p: SYN with 0 vtid", pkt);
            err = TS_ERR_INVALID;
            break;
        }
#endif
        // Update params in receive window
        recvWin->cur_ack = pkt->GetSeqNum() + 1;
        recvWin->head_seq = recvWin->cur_ack;

        synAckPkt = new (std::nothrow) Ts2::PacketSynAck(
                                                         src_udi.deviceId, src_udi.instanceId, lcl_vt_id, 
                                                         dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id, 
                                                         sendWin->cur_seq,
                                                         recvWin->cur_ack,
                                                         recvWin->window_capacity - recvWin->recvSegments_size);
        if (synAckPkt == NULL) {
            LOG_ERROR("Allocation failure!");
            // Probably can't continue in this case
            err = TS_ERR_NO_MEM;
            break;
        }
#ifdef TS2_PKT_RETRY_ENABLE
        // Spawn retry thread for packet retransmission
        if (!isRetryThreadStarted) {
            LOG_DEBUG("Spawn retry thread for reliable transmission...");
            err = Util_SpawnThread(retryThread, this, threadStackSize, /*isJoinable*/VPL_TRUE, &retryHandle);
            if (err != VPL_OK) {
                LOG_ERROR("Spawn retry thread fails: %d", err);
                err = TS_ERR_INTERNAL;
                delete synAckPkt;
                break;
            }
            isRetryThreadStarted = true;
        }
        // insert to segment list
        {
            // Note: this nests the segs_mutex within the buffers_mutex.
            // Make sure this is the only order in which these are nested.
            MutexAutoLock lock(&sendWin->segs_mutex);
    
            send_segment *seg = new (std::nothrow) send_segment;
            if (seg == NULL) {
                err = TS_ERR_NO_MEM;
                LOG_ERROR("Allocation failure: %d", err);
                break;
            }
            // maintain information of SYN ACK packet
            // so we can re-contruct SYN ACK packet during retransmission
            seg->pktType = TS_PKT_SYN_ACK;
            seg->seqNum = synAckPkt->GetSeqNum();
            seg->ackNum = synAckPkt->GetAckNum();
            seg->data = NULL;
            seg->dataSize = 0;
            seg->sendTimestamp = VPLTime_GetTimeStamp();
            seg->retryRound = 1;  // default retry round is 1 (for retry interval exponential back-off)
            seg->retryInterval = sendWin->GetCurrentDefaultRetryInterval();  // default retry interval of each segment
            seg->head_seq_index = seg->seqNum;  // for later segment cleanup after ACK received
            err = sendWin->InsertSegments(seg);
            if (err != TS_OK) {
                // clean up segment and synPkt if failed
                LOG_ERROR("Segment insertion failure, %d", err);
                delete synAckPkt;
                delete seg;
                break;
            }
        }
#endif
        // enqueue packet to FrontEnd
        LOG_DEBUG("Sending SYN_ACK packet %p from lcl "FMTu32" to rmt "FMTu32, synAckPkt, lcl_vt_id, rmt_vt_id);
        sendPacket(synAckPkt, sendWin->GetCurrentDefaultRetryInterval());
        // The lower layers will free the synAckPkt once it is transmitted
        synAckPkt = NULL;

    } while (ALWAYS_FALSE_CONDITIONAL); // end of autolock scope

    return err;
}

void Ts2::Transport::Vtunnel::reSendAck(u64 ackNo)
{
    assert(VPLMutex_LockedSelf(&recvWin->segs_mutex));

    do {
        int err = 0;
        Ts2::PacketAck* ackPkt = NULL;

        // XXX: reference to sendWin but not holding that lock?
        ackPkt = new (std::nothrow) Ts2::PacketAck(
                                                   src_udi.deviceId, src_udi.instanceId, lcl_vt_id,
                                                   dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id,
                                                   sendWin->cur_seq, ackNo,
                                                   recvWin->window_capacity - recvWin->recvSegments_size);
        if (ackPkt == NULL) {
            err = TS_ERR_NO_MEM;
            break;
        }
        LOG_INFO("Vtunnel["FMTu32"]: rexmit ACK packet %p with seq: "FMTu64", ack: "FMTu64,
                  lcl_vt_id, ackPkt, sendWin->cur_seq, ackNo);
        sendPacket(ackPkt, 0);

    } while (ALWAYS_FALSE_CONDITIONAL);

    return;
}

void Ts2::Transport::Vtunnel::handleFin(Packet *pkt)
{
    int err = 0;
    Ts2::PacketFinAck* finAckPkt = NULL;
    u64 currentAck;
    u64 currentWindow;

    assert(VPLMutex_LockedSelf(&mutex));

    //
    // The other end of the tunnel has initiated Close processing
    //
    LOG_DEBUG("Got FIN %p for lcl %u rmt %u", pkt, lcl_vt_id, rmt_vt_id);

    //
    // Note that we specifically do not clear the recv queue.  If there is data
    // still to be consumed, the reader must be allowed to read it before it calls
    // WaitForClose() to finish the close.  Wake the reader, in case it is sitting
    // there with an empty queue.
    //
    {
        // Nests recvWindow segs_mutex in overall mutex
        MutexAutoLock recv_lock(&recvWin->segs_mutex);
        if (recvWin->recvThreadCount > 0) {
            LOG_INFO("Waking %d pending reader(s)", recvWin->recvThreadCount);
            recvWin->closeInProgress = true;
            VPLCond_Broadcast(&recvWin->recv_cond);
        }
        // Calculate the ack number and window for the upcoming FIN_ACK, while
        // we hold the recv lock.
        currentAck = recvWin->cur_ack = pkt->GetSeqNum() + 1;
        currentWindow = recvWin->window_capacity - recvWin->recvSegments_size;
    }

    // Clear the write queue and wake pending writers
    sendWin->Clear();

    //
    // TODO: this will not really work if the FIN packet arrives out-of-order.  Subsequent
    // data packets could be recognized by the sequence numbers, but we aren't really
    // checking that here. This logic needs to be added if we really want out-of-order to work.
    //

    // Reply with FIN ACK
    finAckPkt = new (std::nothrow) Ts2::PacketFinAck(
                                                     src_udi.deviceId, src_udi.instanceId, lcl_vt_id,
                                                     dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id,
                                                     sendWin->cur_seq,
                                                     currentAck,
                                                     currentWindow);
    if (finAckPkt == NULL) {
        LOG_ERROR("Allocation failure!");
        // Probably can't continue in this case
        err = TS_ERR_NO_MEM;
        goto end;
    }
    LOG_DEBUG("Send FIN_ACK %p for lcl %u rmt %u with ack num "FMTu64, finAckPkt, lcl_vt_id, rmt_vt_id, currentAck);
    sendPacket(finAckPkt, 0);

    //
    // There is a race between the FIN arriving from the client and the call
    // to WaitForClose from the server side of the connection.  If the current
    // state is still VT_STATE_OPEN, that means the FIN arrived first.
    //
    if (state == VT_STATE_OPEN) {
        state = VT_STATE_CLOSING;
    } else if (state == VT_STATE_CLOSING || state == VT_STATE_CLOSE_WAIT) {
        state = VT_STATE_CLOSED;
        VPLCond_Broadcast(&close_cond);
    }

end:
    delete pkt;
}

void Ts2::Transport::Vtunnel::handleFinAck(Packet *pkt)
{
    assert(VPLMutex_LockedSelf(&mutex));

    do {
        if (rmt_vt_id != pkt->GetSrcVtId()) {
            LOG_ERROR("Drop Packet %p: src vtunnel id invalid ("FMTu32" != rmt "FMTu32,
                       pkt, pkt->GetSrcVtId(), rmt_vt_id);
            break;
        }
        if (state != VT_STATE_CLOSING && state != VT_STATE_CLOSE_WAIT) {
            LOG_ERROR("Drop Packet %p: bad vtunnel state %u for vt "FMTu32, pkt, state, lcl_vt_id);
            break;
        }
        // verify ack (XXX need a lock)
        if (pkt->GetAckNum() != sendWin->cur_seq + 1) {
            LOG_WARN("FIN_ACK %p: ack mismatch ("FMTu64" != "FMTu64")", pkt, pkt->GetAckNum(), sendWin->cur_seq+1);
            // TODO: should we really fail if ack doesn't match?
            //break;
        }
        // Got the FIN ACK, good to go!
        LOG_DEBUG("Vtunnel[%p]: Got FIN_ACK Packet %p: close complete for lcl "FMTu32" rmt "FMTu32,
            this, pkt, lcl_vt_id, rmt_vt_id);
        state = VT_STATE_CLOSED;
#ifdef TS2_PKT_RETRY_ENABLE
        // Stop writer and retry threads
        stopWorkerThreads();
        // Clean up sendWin, which will abort any waiting writers
        sendWin->Clear();
#endif
        VPLCond_Broadcast(&close_cond);
    } while (ALWAYS_FALSE_CONDITIONAL);

    delete pkt;
}

void Ts2::Transport::Vtunnel::handleThreeWayAck(Packet *pkt, bool bDeletePkt)
{
    do {
#ifndef TS2_PKT_RETRY_ENABLE
        // Check packet type == ACK
        if (pkt->GetPktType() != TS_PKT_ACK) {
            LOG_ERROR("Drop Packet %p: not an ACK (type %u)", pkt, pkt->GetPktType());
#else
        // Check packet type == ACK or DATA
        if (pkt->GetPktType() != TS_PKT_ACK && pkt->GetPktType() != TS_PKT_DATA) {
            LOG_ERROR("Drop Packet %p: not an ACK or DATA (type %u)", pkt, pkt->GetPktType());
#endif
            break;
        }
        if (pkt->GetAckNum() != sendWin->cur_seq + 1) {
            LOG_ERROR("Drop Packet %p: not a valid ack number (pkt ack "FMTu64", cur_seq "FMTu64")", pkt, pkt->GetAckNum(), sendWin->cur_seq);
            break;
        }
        if (pkt->GetSeqNum() != recvWin->cur_ack) {
            LOG_ERROR("Drop Packet %p: not a valid sequence number (pkt seq "FMTu64", cur_ack "FMTu64")", pkt, pkt->GetSeqNum(), recvWin->cur_ack);
            break;
        }

        LOG_DEBUG("Got ACK Packet %p: three-way handshake complete", pkt);
        LOG_DEBUG("seq: "FMTu64", ack: "FMTu64, pkt->GetSeqNum(), pkt->GetAckNum());

#ifdef TS2_ENABLE_RTT_BASED_RETRY
        map<u64, send_segment*>::iterator it = sendWin->sendSegments.find(sendWin->cur_seq);
        if (it != sendWin->sendSegments.end()) {
            // Update round trip time of sendWindow when sequence number matches
            double rate = 0.1;
            VPLTime_t rtt = VPLTime_DiffClamp(VPLTime_GetTimeStamp(), it->second->sendTimestamp);
            sendWin->currRTT = (1 - rate) * sendWin->currRTT + rate * rtt;
            if (sendWin->currRTT < localInfo->GetMinRetransmitInterval()) {
                sendWin->currRTT = localInfo->GetMinRetransmitInterval();
                LOG_DEBUG("Current RTT is too short, updated to minimum retransmission interval: "FMTu64, sendWin->currRTT);
            } else if (sendWin->currRTT > localInfo->GetRetransmitInterval()) {
                sendWin->currRTT = localInfo->GetRetransmitInterval();
                LOG_DEBUG("Current RTT is too large, updated to default retransmission interval: "FMTu64, sendWin->currRTT);
            } else {
                LOG_DEBUG("Current RTT updated: "FMTu64" by RTT: "FMTu64, sendWin->currRTT, rtt);
            }
        }
#endif
        // Init window size from SYN ACK
        sendWin->SetWindowSize(pkt->GetWindowSize());
        // Init seq & ack number
        sendWin->nextBufferIndex = sendWin->currBufferIndex = sendWin->cur_seq = pkt->GetAckNum();

        // Good to go!  Reset state and wake up the opening thread from WaitForOpen
        state = VT_STATE_OPEN;
#ifdef TS2_PKT_RETRY_ENABLE
        // Cancel non-DATA packet in sendSegments & retrySegments
        sendWin->CancelCtrlPktFromSegments();
#endif
        // The main mutex will be released when we return to the caller
        VPLCond_Signal(&open_cond);

    } while (ALWAYS_FALSE_CONDITIONAL);

    if (bDeletePkt) {
        delete pkt;
    }

    return;
}

void Ts2::Transport::Vtunnel::handleAck(Packet *pkt)
{
    u64 latest_ack = pkt->GetAckNum();

    LOG_DEBUG("VT["FMTu32"]: Handling ACK packet %p (seq: "FMTu64", ack: "FMTu64"), sendWin cur_seq: "FMTu64,
        this->lcl_vt_id, pkt, pkt->GetSeqNum(), latest_ack, sendWin->cur_seq);

    // XXX no lock held yet.  is it safe to mess with sendWin?
    if (latest_ack > sendWin->cur_seq) {
        // FIXME: when we start handling out-of-order pkts, this won't work
        {
            MutexAutoLock segLock(&sendWin->segs_mutex);
            sendWin->cur_seq = latest_ack;
            LOG_DEBUG("Dump sendSegments[%p] (size = "FMTu_size_t"): ", &sendWin->sendSegments, sendWin->sendSegments.size());
            {
                map<u64, send_segment*>::iterator it = sendWin->sendSegments.begin();
                for ( ; it != sendWin->sendSegments.end(); it++) {
#ifndef TS2_PKT_RETRY_ENABLE
                    LOG_DEBUG("\t\tpacket[%p] head seq: "FMTu64", data len: "FMTu32,
                        it->second->pkt,
                        it->second->pkt->GetSeqNum(), it->second->pkt->GetDataSize());
#else
                    LOG_DEBUG("\t\tpacket["FMTu64"], data len: "FMTu32,
                        it->second->seqNum, it->second->dataSize);
#endif
                }
            }
            {
                map<u64, send_segment*>::iterator it_seg = sendWin->sendSegments.begin();
                while (it_seg != sendWin->sendSegments.end()) {
                    // cleanup sendSegments as far as possible
#ifndef TS2_PKT_RETRY_ENABLE
                    u64 segment_ack = it_seg->second->pkt->GetSeqNum() + it_seg->second->pkt->GetDataSize();
                    LOG_DEBUG("sendSegments["FMTu64"]: data len in data pkt: "FMTu32"\n"
                        "\t\t\t\t\t\t\tsegment expected ack:"FMTu64,
                        it_seg->second->pkt->GetSeqNum(),
                        it_seg->second->pkt->GetDataSize(), segment_ack);
#else
                    u64 segment_ack = it_seg->second->seqNum + it_seg->second->dataSize;
                    LOG_DEBUG("sendSegments["FMTu64"]: data len in data pkt: "FMTu32"\n"
                        "\t\t\t\t\t\t\tsegment expected ack:"FMTu64,
                        it_seg->second->seqNum,
                        it_seg->second->dataSize, segment_ack);
#endif
#ifdef TS2_ENABLE_RTT_BASED_RETRY
                    if (latest_ack == segment_ack) {
                        // Update round trip time of sendWindow when sequence number matches
                        double rate = 0.1;
                        VPLTime_t rtt = VPLTime_DiffClamp(VPLTime_GetTimeStamp(), it_seg->second->sendTimestamp);
                        sendWin->currRTT = (1 - rate) * sendWin->currRTT + rate * rtt;
                        if (sendWin->currRTT < localInfo->GetMinRetransmitInterval()) {
                            sendWin->currRTT = localInfo->GetMinRetransmitInterval();
                            LOG_DEBUG("Current RTT is too short, updated to minimum retransmission interval: "FMTu64, sendWin->currRTT);
                        } else if (sendWin->currRTT > localInfo->GetRetransmitInterval()) {
                            sendWin->currRTT = localInfo->GetRetransmitInterval();
                            LOG_DEBUG("Current RTT is too large, updated to default retransmission interval: "FMTu64, sendWin->currRTT);
                        } else {
                            LOG_DEBUG("Current RTT updated: "FMTu64" by RTT: "FMTu64, sendWin->currRTT, rtt);
                        }
                    }
#endif
                    if (latest_ack >= segment_ack) {
                        // Remove from retrySegments
#ifndef TS2_PKT_RETRY_ENABLE
                        LOG_DEBUG("erase retry at ["FMTu64"] for seg: "FMTu64" from retrySegments",
                            it_seg->second->sendTimestamp + it_seg->second->retryInterval,
                            it_seg->second->pkt->GetSeqNum());
#else
                        LOG_DEBUG("erase retry at ["FMTu64"] for seg: "FMTu64" from retrySegments",
                            it_seg->second->sendTimestamp + it_seg->second->retryInterval,
                            it_seg->second->seqNum);
#endif
                        sendWin->retrySegments.erase(it_seg->second->sendTimestamp + it_seg->second->retryInterval);
                        // Wake the retry thread to update timer if necessary
                        if (it_seg->second->sendTimestamp + it_seg->second->retryInterval <= sendWin->currRetryTimestamp) {
                            VPLCond_Broadcast(&sendWin->retry_cond);
                        }
                        // Remove from sendSegments
#ifndef TS2_PKT_RETRY_ENABLE
                        LOG_DEBUG("sendSegments size: "FMTu64", items: "FMTu_size_t", remove size: "FMTu32, sendWin->sendSegments_size, sendWin->sendSegments.size(), it_seg->second->pkt->GetDataSize());
                        sendWin->sendSegments_size -= it_seg->second->pkt->GetDataSize();
#else
                        LOG_DEBUG("sendSegments size: "FMTu64", items: "FMTu_size_t", remove size: "FMTu32, sendWin->sendSegments_size, sendWin->sendSegments.size(), it_seg->second->dataSize);
                        sendWin->sendSegments_size -= it_seg->second->dataSize;
#endif
#ifndef TS2_PKT_RETRY_ENABLE
                        // delete acknowledged packet
                        delete it_seg->second->pkt;
#else
                        // release data pointer for acknowledged packet
                        it_seg->second->data = NULL;
#endif
                        // delete send_segment object
                        delete it_seg->second;
                        map<u64, send_segment*>::iterator it_cur = it_seg++;
                        sendWin->sendSegments.erase(it_cur);
                        LOG_DEBUG("after removal, sendSegments size: "FMTu64", items: "FMTu_size_t, sendWin->sendSegments_size, sendWin->sendSegments.size());
                        // Wake the writer thread in case the window was full
                        VPLCond_Signal(&sendWin->seg_cond);
                        // FIXME: does this work once we start supporting SACK?
                    }
                    else {
                        // Since sendSegments are in order in the map,
                        // we are done if latest ack is smaller than segment ack
                        LOG_DEBUG("ACK packet[%p] done: next expected ACK "FMTu64, pkt, segment_ack);
                        break;
                    }
                }
            }
        }
        {
            MutexAutoLock bufLock(&sendWin->buffers_mutex);
            map<u64, send_buffer*>::iterator it_buf = sendWin->sendBuffers.begin();
            while (it_buf != sendWin->sendBuffers.end()) {
                // cleanup sendBuffers as far as possible
                u64 buffer_ack = it_buf->second->head_seq + it_buf->second->buffer_size;
                LOG_DEBUG("buffer len: "FMTu64"\n"
                    "\t\t\t\t\t\t\tbuffer expected ack:"FMTu64,
                    (u64)it_buf->second->buffer_size,
                    buffer_ack);
                if (latest_ack >= buffer_ack) {
                    LOG_DEBUG("sendBuffers size: "FMTu64", items:"FMTu_size_t, sendWin->sendBuffers_size, sendWin->sendBuffers.size());
                    sendWin->sendBuffers_size -= it_buf->second->buffer_size;
                    // Wake the caller of Write(), since buffer write is complete
                    VPLCond_Signal(it_buf->second->hComplete);
                    // delete send_buffer object
                    delete it_buf->second;
                    map<u64, send_buffer*>::iterator it_cur = it_buf++;
                    sendWin->sendBuffers.erase(it_cur);
                    LOG_DEBUG("sendBuffers size: "FMTu64", items:"FMTu_size_t, sendWin->sendBuffers_size, sendWin->sendBuffers.size());
                } else {
                    // Since sendBuffers are in order,
                    // we are done when latest ack is smaller than buffer ack
                    break;
                }
            }
        }
    }
    else {
        // Duplicate ACK may mean that some packets were dropped
        LOG_INFO("ACK packet[%p] already handled, drop it! latest_ack: "FMTu64", current seq: "FMTu64, pkt, latest_ack, sendWin->cur_seq);
    }

    delete pkt;
}

void Ts2::Transport::Vtunnel::handleData(Packet *pkt)
{
    map<u64, PacketData*>::iterator it;
    PacketData* dataPkt = (PacketData*)pkt;
    bool isNewData = false;
    u64 pkt_seq = dataPkt->GetSeqNum();
    u64 data_size = dataPkt->GetDataSize();

    MutexAutoLock lock(&recvWin->segs_mutex);

    if (data_size == 0) {
        goto end;
    }

    LOG_DEBUG("Vtunnel["FMTu32"]: recvWin->cur_ack: "FMTu64", pkt seq: "FMTu64", recvSegments_size "FMTu64,
        lcl_vt_id, recvWin->cur_ack, pkt_seq, recvWin->recvSegments_size);
    if (recvWin->cur_ack == pkt_seq) {
        // packet is the next expected one
        // received packets are out of order and queued in recvWin
        // update ack number as large as possible
        recvWin->cur_ack += data_size;
        LOG_DEBUG("recvWin[%p] head_seq: "FMTu64", cur_ack: "FMTu64, recvWin, recvWin->head_seq, recvWin->cur_ack);
        while((it = recvWin->recvSegments.find(recvWin->cur_ack)) != recvWin->recvSegments.end()) {
            recvWin->cur_ack += it->second->GetDataSize();
            LOG_DEBUG("recvWin[%p] head_seq: "FMTu64", cur_ack: "FMTu64, recvWin, recvWin->head_seq, recvWin->cur_ack);
        }
    } else if (recvWin->cur_ack > pkt_seq) {
        LOG_WARN("Vtunnel["FMTu32"]: out of order packet (and already been received before), seq "FMTu64", len "FMTu64, lcl_vt_id, pkt_seq, data_size);
        // out of order packet seq number
        // and already been received before
        // packet comes from different links, so we always reply cur_ack to make progress
        reSendAck(recvWin->cur_ack);
        goto end;
    }

    if (recvWin->recvSegments.find(pkt_seq) == recvWin->recvSegments.end()) {
        // append to recvSegments if not found
        // seq might be cur_ack or larger than cur_ack
        recvWin->recvSegments[pkt_seq] = dataPkt;
        recvWin->recvSegments_size += data_size;
        // hand over packet ownership to the recvSegments
        pkt = NULL;
        dataPkt = NULL;
        isNewData = true;
    } else {
        LOG_INFO("Vtunnel["FMTu32"]: pkt seq "FMTu64" already in recvSegs", lcl_vt_id, pkt_seq);
        // packet is already been received more than once
        // assume remote didn't receive ack packet, need to reply cur_ack to make progress
        reSendAck(recvWin->cur_ack);
        goto end;
    }

    // reply ack
    {
        Ts2::PacketAck* ackPkt = NULL;
        TSError_t err = TS_OK;
        // XXX: reference to sendWin but not holding that lock?
        ackPkt = new (std::nothrow) Ts2::PacketAck(
                                                   src_udi.deviceId, src_udi.instanceId, lcl_vt_id,
                                                   dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id,
                                                   sendWin->cur_seq, recvWin->cur_ack,
                                                   recvWin->window_capacity - recvWin->recvSegments_size);
        if (ackPkt == NULL) {
            err = TS_ERR_NO_MEM;
            LOG_ERROR("Allocation failure: %d", err);
            goto end;
        }
        LOG_DEBUG("Vtunnel["FMTu32"]: sending ACK packet %p with seq: "FMTu64", ack: "FMTu64,
                  lcl_vt_id, ackPkt, sendWin->cur_seq, recvWin->cur_ack);
        sendPacket(ackPkt, 0);
    }

 end:
    // Notify reader if new data to read
    if (isNewData) {
        VPLCond_Signal(&recvWin->recv_cond);
    }
    else {
        // Cleanup data packet if it is a duplicate
        dataPkt = NULL;
        delete pkt;
    }
    return;
}

// Handler for Incoming Packets to this Vtunnel
void Ts2::Transport::Vtunnel::ReceivePacket(Packet* pkt)
{
    do {
        MutexAutoLock lock(&mutex);
        // State check
        if (state == VT_STATE_NULL || state == VT_STATE_CLOSED) {
            // TODO: add a RST packet to the protocol and send that if a DATA packet
            //       is received on a closed or non-existent tunnel
            LOG_DEBUG("VT["FMTu32"]: Wrong tunnel state %d, drop packet %p type "FMTu8, 
                lcl_vt_id, state, pkt, pkt->GetPktType());
            delete pkt;
            break;
        }
        LOG_DEBUG("VT["FMTu32"] packet[%p]: type "FMTu8, lcl_vt_id, pkt, pkt->GetPktType());
        // If the tunnel is opening, open or closing, queue the packet for handling and wake
        // the application thread to process it.
        switch (pkt->GetPktType()) {
        case TS_PKT_SYN:
            // Shouldn't happen, TS_PKT_SYN is handled by Pool::connRequestHandler()
            LOG_WARN("drop SYN packet[%p] to open connection, connection is already setup", pkt);
            delete pkt;
            break;
        case TS_PKT_FIN:
            // TODO: Bug 18335: use ack number in FIN packet to acknowledge sendSegments
            //VPLMutex_Unlock(&mutex);
            //handleAck(pkt);
            //VPLMutex_Lock(&mutex);
            // Close initiated by other end of the tunnel
            handleFin(pkt);
            break;
        case TS_PKT_FIN_ACK:
            handleFinAck(pkt);
            break;
        case TS_PKT_SYN_ACK:
#ifdef TS2_PKT_RETRY_ENABLE
            // TODO: When TS enables ctrl packet retransmission, old server sometimes receives redundant SYN packets for one open attempt
            // server side then creates multiple vtunnels in VT_STATE_HALF_OPEN state for one open attempt
            // to avoid old server finishes three-way handshaking, we drops redundant SYN ACK for the same same vtunnel here
            // so those VT_STATE_HALF_OPEN vtunnels will eventually timeout and been closed
            if (state == VT_STATE_OPENING) {
#endif
                openQueue.push_back(pkt);
                VPLCond_Signal(&open_cond);
#ifdef TS2_PKT_RETRY_ENABLE
            }
#endif
            break;
        case TS_PKT_ACK:
            if (state == VT_STATE_HALF_OPEN) {
                VPLMutex_Unlock(&mutex);
                handleThreeWayAck(pkt);
                VPLMutex_Lock(&mutex);
            } else if (state == VT_STATE_OPEN ||
                       state == VT_STATE_CLOSING ||
                       state == VT_STATE_CLOSE_WAIT) {
                // Handle Acknowledgement
                VPLMutex_Unlock(&mutex);
                handleAck(pkt);
                VPLMutex_Lock(&mutex);
            } else {
                LOG_WARN("VT["FMTu32"]: drop ACK, bad tunnel state %u", lcl_vt_id, state);
                delete pkt;
            }
            break;
        case TS_PKT_DATA:

#ifdef TS2_PKT_RETRY_ENABLE
            // TODO:
            // Half open tunnels on the server side verify the correctness of ack number in DATA packet
            // If ack number is correct, it implies the sequence numbers are correctly exchanged
            // and the three way handshake is regarded as success
            // If multiple routes are in use, there is a race between the ACK and the first
            // DATA packet. Both ACK and DATA packet can be the last ACK in three way handshake now.
            if (state == VT_STATE_HALF_OPEN) {
                VPLMutex_Unlock(&mutex);
                LOG_WARN("VT["FMTu32"]: handle DATA packet %p as the last three way handshake ACK", lcl_vt_id, pkt);
                handleThreeWayAck(pkt, false);
                VPLMutex_Lock(&mutex);
            }
            if (state == VT_STATE_CLOSING) {
                LOG_WARN("VT["FMTu32"]: drop DATA packet %p, bad tunnel state %u", lcl_vt_id, pkt, state);
                delete pkt;
                break;
            }
#else
            //
            // Half open tunnels on the server side can not receive data yet, since the
            // final ACK of the three way handshake is needed to get sequence numbers in sync.
            // If multiple routes are in use, there is a race between the ACK and the first
            // DATA packet.  If the ACK loses, then the sender will have to retransmit the
            // DATA packet.
            //
            if (state == VT_STATE_HALF_OPEN || state == VT_STATE_CLOSING) {
                LOG_WARN("VT["FMTu32"]: drop DATA packet %p, bad tunnel state %u", lcl_vt_id, pkt, state);
                delete pkt;
                break;
            }
#endif
            VPLMutex_Unlock(&mutex);
            handleData(pkt);
            VPLMutex_Lock(&mutex);
            break;
        default:
            break;
        }

    } while (ALWAYS_FALSE_CONDITIONAL);

    return;
}

// Send Packets to this Vtunnel
void Ts2::Transport::Vtunnel::sendPacket(Packet *pkt, VPLTime_t pktTTL)
{
    int err = TS_OK;

    //
    // No mutex is needed here, since networkFrontEnd is a word in memory and can't
    // be modified once it is non-NULL.
    //
    if (networkFrontEnd != NULL) {
#ifdef TS2_ENABLE_RTT_BASED_RETRY
        if (pktTTL == 0)
            err = networkFrontEnd->Enqueue(pkt, localInfo->GetPacketSendTimeout());
        else
            err = networkFrontEnd->Enqueue(pkt, pktTTL);
#else
        err = networkFrontEnd->Enqueue(pkt, localInfo->GetPacketSendTimeout());
#endif
    } else {
        LOG_ERROR("Vtunnel[%p]: No enqueue function for vt "FMTu32, this, lcl_vt_id);
        err = TS_ERR_INVALID;
    }

    // If the packet is successfully enqueued, then the lower layers will free it.
    // If not, then it happens here, since the caller assumes it is gone.
    if (err != TS_OK) {
        delete pkt;
    }
    return;
}

int Ts2::Transport::Vtunnel::Close(string& err_msg)
{
    int err = 0;

    err_msg.clear();
    do {
        MutexAutoLock lock(&mutex);
        // State check and transition.
        if (state == VT_STATE_CLOSED) {
            LOG_DEBUG("Vtunnel[%p]: vt "FMTu32" already closed", this, lcl_vt_id);
            break;
        }
        if (state != VT_STATE_OPEN && state != VT_STATE_CLOSING) {
            LOG_WARN("Vtunnel[%p]: vt "FMTu32" wrong tunnel state %d", this, lcl_vt_id, state);
            err = TS_ERR_WRONG_STATE;
            break;
        }
        // If the FIN has already been received from the other side
        if (state == VT_STATE_CLOSING) {
            // Shutdown the IO threads and then we are done
            closeTunnel(/*sendFin*/false);
            state = VT_STATE_CLOSED;
            // In case Close was called twice (TS-EXT does this)
            VPLCond_Broadcast(&close_cond);
            break;
        }

        // This end of the tunnel is initiating the Close()
        state = VT_STATE_CLOSING;
    
        // Stop IO threads and send FIN to notify the other end of the tunnel
        closeTunnel(/*sendFin*/true);

        // Wait for the FIN-ACK response from the remote side or a FIN
        LOG_DEBUG("Vtunnel[%p]: wait for Close to complete for lcl "FMTu32", rmt "FMTu32,
                  this, lcl_vt_id, rmt_vt_id);
        while (state == VT_STATE_CLOSING) {
            err = VPLCond_TimedWait(&close_cond, &mutex, localInfo->GetTsCloseTimeout());
            if (err == VPL_ERR_TIMEOUT) {
                LOG_ERROR("Waited %d ms for FIN-ACK, force close lcl "FMTu32", rmt "FMTu32,
                          (int)VPLTime_ToMillisec(localInfo->GetTsCloseTimeout()), lcl_vt_id, rmt_vt_id);
                err = TS_ERR_TIMEOUT;
                state = VT_STATE_CLOSED;
                break;
            }
            if (err) {
                LOG_ERROR("Vtunnel[%p]: CondVar failed: err %d", this, err);
                break;
            }
        }

        if (state != VT_STATE_CLOSED) {
            err = TS_ERR_NO_CONNECTION;
        }
    } while (ALWAYS_FALSE_CONDITIONAL);

    return err;
}

//
// Common code for client and server Close processing
//
void Ts2::Transport::Vtunnel::closeTunnel(bool sendFin)
{
    assert(VPLMutex_LockedSelf(&mutex));
    LOG_DEBUG("Vtunnel[%p]: close tunnel lcl "FMTu32", rmt "FMTu32, this, lcl_vt_id, rmt_vt_id);

    do {
        if (state != VT_STATE_OPEN && state != VT_STATE_CLOSING) {
            LOG_WARN("Vtunnel[%p]: Wrong tunnel state %d", this, state);
            break;
        }

#ifndef TS2_PKT_RETRY_ENABLE
        //
        // Note that this behavior is different than TCP, because Write is synchronous
        // at the app level in the TS2 implementation.  If the app is calling Close, then
        // any pending writes have either completed or failed.  In the TCP case, there
        // may still be outgoing data on the socket and you want the FIN to wait for that.
        //

        // Stop writer and retry threads
        stopWorkerThreads();
#else
        // Stop writer and retry threads if no need to send FIN
        if (!sendFin) {
            stopWorkerThreads();
        }
#endif
        // Clean up sendWin, which will abort any waiting writers
        sendWin->Clear();

        // Check for any pending reads and abort them
        {
            // Nests recvWindow segs_mutex in overall mutex
            MutexAutoLock recv_lock(&recvWin->segs_mutex);
            if (recvWin->recvThreadCount > 0) {
                LOG_INFO("Aborting %d pending readers", recvWin->recvThreadCount);
                recvWin->closeInProgress = true;
                VPLCond_Broadcast(&recvWin->recv_cond);
            }
        }

        if (sendFin) {
            // Send FIN packet
            Ts2::PacketFin *finPkt = new (std::nothrow) Ts2::PacketFin(
                                                                       src_udi.deviceId, src_udi.instanceId, lcl_vt_id,
                                                                       dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id,
                                                                       sendWin->cur_seq,
                                                                       recvWin->cur_ack);
            if (finPkt == NULL) {
                LOG_ERROR("Unable to allocate FIN packet vt "FMTu32, lcl_vt_id);
                break;
            }
#ifdef TS2_PKT_RETRY_ENABLE
            // insert to segment list
            {
                // Note: this nests the segs_mutex within the buffers_mutex.
                // Make sure this is the only order in which these are nested.
                MutexAutoLock lock(&sendWin->segs_mutex);

                send_segment *seg = new (std::nothrow) send_segment;
                if (seg == NULL) {
                    LOG_ERROR("Allocation failed");
                    break;
                }
                // maintain information of FIN packet
                // so we can re-contruct FIN packet during retransmission
                seg->pktType = TS_PKT_FIN;
                seg->seqNum = finPkt->GetSeqNum();
                seg->ackNum = finPkt->GetAckNum();
                seg->data = NULL;
                seg->dataSize = 0;
                seg->sendTimestamp = VPLTime_GetTimeStamp();
                seg->retryRound = 1;  // default retry round is 1 (for retry interval exponential back-off)
                seg->retryInterval = sendWin->GetCurrentDefaultRetryInterval();  // default retry interval of each segment
                seg->head_seq_index = seg->seqNum;  // for later segment cleanup after FIN ACK received
                int err = sendWin->InsertSegments(seg);
                if (err != TS_OK) {
                    // clean up segment and synPkt if failed
                    LOG_ERROR("Segment insertion failure, %d", err);
                    delete finPkt;
                    delete seg;
                    break;
                }
            }
#endif
            // enqueue packet to FrontEnd
            LOG_DEBUG("Sending FIN packet %p from lcl "FMTu32" to rmt "FMTu32, finPkt, lcl_vt_id, rmt_vt_id);
            sendPacket(finPkt, sendWin->GetCurrentDefaultRetryInterval());
            // The lower layers will free the finPkt once it is transmitted
            finPkt = NULL;
        }

    } while (ALWAYS_FALSE_CONDITIONAL);

    return;
}

//
// Each active tunnel has two private worker threads
//
void Ts2::Transport::Vtunnel::stopWorkerThreads()
{
    // Stop retry thread if necessary
    if (isRetryThreadStarted) {
        LOG_DEBUG("Vtunnel[%p]: stop retry thread for vt "FMTu32, this, lcl_vt_id);
        {
            MutexAutoLock lock(&sendWin->segs_mutex);
            isRetryStop = true;
            VPLCond_Broadcast(&sendWin->retry_cond);
        }
        VPLDetachableThread_Join(&retryHandle);
        isRetryThreadStarted = false;
    }
    // Stop writer thread if necessary
    if (isWriterStarted) {
        LOG_DEBUG("Vtunnel[%p]: stop writer thread for vt "FMTu32, this, lcl_vt_id);
        //
        // Note that the writer thread may have spontaneously exited in error cases in which
        // case isWriterStop will already be true, but the join still needs to happen.
        //
        if (!isWriterStop) {
            isWriterStop = true;
            {
                MutexAutoLock lock(&sendWin->segs_mutex);
                VPLCond_Broadcast(&sendWin->seg_cond);
            }
            {
                MutexAutoLock lock(&sendWin->buffers_mutex);
                VPLCond_Broadcast(&sendWin->buffer_cond);
            }
        }
        VPLDetachableThread_Join(&writerHandle);
        isWriterStarted = false;
    }
}

// Class method (declared static in the header)
VPLTHREAD_FN_DECL Ts2::Transport::Vtunnel::writerThread(void* arg)
{
    Ts2::Transport::Vtunnel *vt = (Ts2::Transport::Vtunnel*)arg;

    vt->writerThread();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

void Ts2::Transport::Vtunnel::writerThread()
{
    TSError_t err = TS_OK;

    while (!isWriterStop) {
        std::map<u64, send_buffer*>::iterator it;

        LOG_DEBUG("VT["FMTu32"] sendWin[%p]: buffer size: "FMTu64", segment size: "FMTu64", window capacity: "FMTu64
             ", currBufferIndex: "FMTu64", currBufferOffset: "FMTu64", nextBufferIndex: "FMTu64,
            this->lcl_vt_id,
            sendWin,
            sendWin->sendBuffers_size,
            sendWin->sendSegments_size,
            sendWin->window_capacity,
            sendWin->currBufferIndex,
            sendWin->currBufferOffset,
            sendWin->nextBufferIndex);

        VPLMutex_Lock(&sendWin->segs_mutex);
        while (!sendWin->isAvailableWindow() && !isWriterStop) {
            // wait until any sent segment is acked
            LOG_DEBUG("Sleep waiting for available Send Window");
            VPLCond_TimedWait(&sendWin->seg_cond, &sendWin->segs_mutex, VPL_TIMEOUT_NONE);
            LOG_DEBUG("Awakened after waiting for available Send Window");
        }
        VPLMutex_Unlock(&sendWin->segs_mutex);

        MutexAutoLock lock(&sendWin->buffers_mutex);
        while (!sendWin->isAvailableBufferToSend() && !isWriterStop) {
            // wait until something to send
            LOG_DEBUG("Sleep waiting for Data to Send");
            VPLCond_TimedWait(&sendWin->buffer_cond, &sendWin->buffers_mutex, VPL_TIMEOUT_NONE);
        }
        if (isWriterStop) {
            break;
        }
        it = sendWin->sendBuffers.find(sendWin->currBufferIndex);
        if (it == sendWin->sendBuffers.end()) {
            // Shouldn't happen. But if it does, it becomes a tight loop unless we bail here.
            LOG_ERROR("Cannot find current buffer to send");
            err = TS_ERR_INTERNAL;
            break;
        }
        else {
            // create segment and insert to segment list
            // create data packet
            Ts2::PacketData* dataPkt = NULL;
            send_buffer* sndBuffer = it->second;

            // segment size = min (sendWin's segment size, current buffer remaining size)
            u64 remaining_buffer = sndBuffer->buffer_size - sendWin->currBufferOffset;
            LOG_DEBUG("remaining buffer: "FMTu64, remaining_buffer);
            // segment size is never bigger than _UI32_MAX
            u32 data_size = MIN((u32)MIN(remaining_buffer, UINT32_MAX), sendWin->GetSegmentSize());
            LOG_DEBUG("data_size: "FMTu32, data_size);
            dataPkt = new (std::nothrow) Ts2::PacketData(
                                                         src_udi.deviceId, src_udi.instanceId, lcl_vt_id,
                                                         dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id,
                                                         sendWin->currBufferIndex + sendWin->currBufferOffset,
                                                         recvWin->cur_ack,
                                                         (u8*)sndBuffer->buffer + sendWin->currBufferOffset,
                                                         data_size,
                                                         recvWin->window_capacity - recvWin->recvSegments_size,
                                                         false);
            if (dataPkt == NULL) {
                // Probably can't continue in this case
                err = TS_ERR_NO_MEM;
                LOG_ERROR("Allocation failure: %d", err);
                break;
            }
            // insert to segment list
            {
                // Note: this nests the segs_mutex within the buffers_mutex.
                // Make sure this is the only order in which these are nested.
                MutexAutoLock lock(&sendWin->segs_mutex);
    
                send_segment *seg = new (std::nothrow) send_segment;
                if (seg == NULL) {
                    err = TS_ERR_NO_MEM;
                    LOG_ERROR("Allocation failure: %d", err);
                    break;
                }
#ifndef TS2_PKT_RETRY_ENABLE
                // duplicate a data packet to insert to segment list
                // so we can have a reference during retransmission
                //PacketHdr_t hdr;
                //dataPkt->DupHdr(hdr);
                PacketData *sndPkt = new (std::nothrow) PacketData(
                                         dataPkt->GetSrcDeviceId(), dataPkt->GetSrcInstId(), dataPkt->GetSrcVtId(),
                                         dataPkt->GetDstDeviceId(), dataPkt->GetDstInstId(), dataPkt->GetDstVtId(),
                                         dataPkt->GetSeqNum(),
                                         dataPkt->GetAckNum(),
                                         dataPkt->GetData(),
                                         data_size,
                                         recvWin->window_capacity - recvWin->recvSegments_size,
                                         false);
                if (sndPkt == NULL) {
                    err = TS_ERR_NO_MEM;
                    LOG_ERROR("Allocation failure: %d", err);
                    delete seg;
                    break;
                }
                seg->pkt = sndPkt;
#else
                // maintain information of data packet
                // so we can re-contruct DATA packet during retransmission
                seg->pktType = TS_PKT_DATA;
                seg->seqNum = dataPkt->GetSeqNum();
                seg->ackNum = dataPkt->GetAckNum();
                seg->data = dataPkt->GetData();
                seg->dataSize = data_size;
#endif
                seg->sendTimestamp = VPLTime_GetTimeStamp();
                seg->retryRound = 1;  // default retry round is 1 (for retry interval exponential back-off)
                seg->retryInterval = sendWin->GetCurrentDefaultRetryInterval();  // default retry interval of each segment
                seg->head_seq_index = sendWin->currBufferIndex;
                err = sendWin->InsertSegments(seg);
                if (err != TS_OK) {
                    // clean up segment and dataPkt if failed
                    LOG_ERROR("Segment insertion failure, %d", err);
                    delete dataPkt;
#ifndef TS2_PKT_RETRY_ENABLE
                    delete sndPkt;
#else
                    seg->data = NULL;
#endif
                    delete seg;
                    break;
                }
#ifndef TS2_PKT_RETRY_ENABLE
                // hand over sndPkt to send window
                sndPkt = NULL;
#endif
            }
            sendWin->currBufferOffset += data_size;
            if (sendWin->currBufferOffset == sndBuffer->buffer_size) {
                // reset offset to 0, and update index to next "expected" buffer
                sendWin->currBufferOffset = 0;
                sendWin->currBufferIndex += sndBuffer->buffer_size;
                LOG_DEBUG("Current buffer is completely sent, update current index to next one: "FMTu64, sendWin->currBufferIndex);
            }
            // enqueue packet to FrontEnd
            LOG_DEBUG("Sending DATA packet %p from lcl "FMTu32" to rmt "FMTu32, dataPkt, lcl_vt_id, rmt_vt_id);
            sendPacket(dataPkt, sendWin->GetCurrentDefaultRetryInterval());
            // The lower layers will free the dataPkt once it is transmitted
            dataPkt = NULL;
        }
    }

    // Set this flag to handle the error cases (breaks from above loop)
    isWriterStop = true;
    LOG_INFO("writerThread exits: lcl vt "FMTu32", err %d", lcl_vt_id, err);
    return;
}

Ts2::Transport::sendWindow::sendWindow(int maxSegmentSize, int maxWindowSize, VPLTime_t retryInterval) :
    sendSegments_size(0),
    sendBuffers_size(0),
    window_capacity(maxWindowSize),
    segment_size(maxSegmentSize),
    cur_seq(0),
    currBufferIndex(0),
    currBufferOffset(0),
    nextBufferIndex(0),
    currRTT(retryInterval),
    currRetryTimestamp(VPL_TIMEOUT_NONE)
{
    VPLMutex_Init(&segs_mutex);
    VPLMutex_Init(&buffers_mutex);
    VPLCond_Init(&seg_cond);
    VPLCond_Init(&buffer_cond);
    VPLCond_Init(&retry_cond);

    // to support SYN packet retransmission
    // sequence number becomes the identity of TS Open attempt
    // randomize sequence number by giving random number to the lowest 16-bits
    // for backward compatibility
    //     sequence number is always = 0 for previous client
    //     sequence number is always > 0 for latest client 
    VPLMath_InitRand();
    cur_seq = ((VPLMath_Rand() & 0xffff) % 0xffff) + 1;
    LOG_INFO("Seq starts from "FMTu64, cur_seq);

    // init segments & buffers
    sendSegments.clear();
    retrySegments.clear();
    sendBuffers.clear();
}

Ts2::Transport::sendWindow::~sendWindow()
{
    VPLMutex_Lock(&segs_mutex);
    // Both maps should be empty, but clear them just in case
    if (sendSegments.size() != 0) LOG_WARN("sendSegments map not empty: "FMTu_size_t" entries", sendSegments.size());
    if (retrySegments.size() != 0) LOG_WARN("retrySegments map not empty: "FMTu_size_t" entries", retrySegments.size());
    sendSegments.clear();
    retrySegments.clear();
    VPLMutex_Unlock(&segs_mutex);

    VPLMutex_Lock(&buffers_mutex);
    sendBuffers.clear();
    VPLMutex_Unlock(&buffers_mutex);

    VPLMutex_Destroy(&segs_mutex);
    VPLMutex_Destroy(&buffers_mutex);
    VPLCond_Destroy(&seg_cond);
    VPLCond_Destroy(&buffer_cond);
    VPLCond_Destroy(&retry_cond);
}

int Ts2::Transport::sendWindow::AppendToBuffers(const void* buffer, size_t buffer_size, VPLCond_t* complete_cond, u64& index)
{
    int rv = TS_OK;
    bool isCurrentBuffer = false;

    do {
        MutexAutoLock autolock(&buffers_mutex);

        LOG_DEBUG("sendWin[%p]: segments size: "FMTu64", buffer size: "FMTu64,
            this,
            sendSegments_size, sendBuffers_size);

        if (sendSegments_size == sendBuffers_size) {
            // This means that all the buffers currently queued have already been
            // sent, so this buffer being appended will be sent next.
            isCurrentBuffer = true;
        }

        if (sendBuffers.find(nextBufferIndex) != sendBuffers.end()) {
            // Shouldn't happen
            LOG_ERROR("buffers["FMTu64"] already exists, shouldn't happen", nextBufferIndex);
            rv = TS_ERR_INTERNAL;
            break;
        }
        send_buffer* sndBuffer = new (std::nothrow) send_buffer;
        if (sndBuffer == NULL) {
            rv = TS_ERR_NO_MEM;
            LOG_ERROR("Allocation error: %d", rv);
            break;
        }
        sndBuffer->buffer = buffer;
        sndBuffer->buffer_size = buffer_size;
        sndBuffer->head_seq = nextBufferIndex;
        sndBuffer->hComplete = complete_cond;
        sndBuffer->sendAborted = false;
        sendBuffers[nextBufferIndex] = sndBuffer;
        index = nextBufferIndex;
        if (isCurrentBuffer) {
            currBufferIndex = nextBufferIndex;
        }

        sendBuffers_size += buffer_size;
        nextBufferIndex += buffer_size;

        LOG_DEBUG("sendWin[%p]: result: %d, segments size: "FMTu64", buffer size: "FMTu64,
            this, rv, sendSegments_size, sendBuffers_size);
        VPLCond_Signal(&buffer_cond);

    } while (ALWAYS_FALSE_CONDITIONAL);

    return rv;
}

void Ts2::Transport::sendWindow::CancelCtrlPktFromSegments()
{
    {
        // Remove all active segments from both sendSegments & retrySegments
        // and wake the retry thread to update the retry timestamp if necessary
        // Note: this nests the segs_mutex within the buffers_mutex.
        MutexAutoLock lock(&segs_mutex);
        bool updateRetryTO = false;
        map<u64, send_segment*>::iterator it = sendSegments.begin();
        // erase element from sendSegments
        while (it != sendSegments.end()) {
            // erase element from retrySegments
#ifndef TS2_PKT_RETRY_ENABLE
            if (it->second->pkt->GetPktType() != TS_PKT_DATA) {
                LOG_WARN("erase segment["FMTu64"] of seg: "FMTu64" from retrySegments",
                    it->second->sendTimestamp + it->second->retryInterval,
                    it->second->pkt->GetSeqNum());
#else
            if (it->second->pktType != TS_PKT_DATA) {
                LOG_WARN("erase segment["FMTu64"] of seg: "FMTu64" from retrySegments",
                    it->second->sendTimestamp + it->second->retryInterval,
                    it->second->seqNum);
#endif
                retrySegments.erase(it->second->sendTimestamp + it->second->retryInterval);
                if (!updateRetryTO && (it->second->sendTimestamp + it->second->retryInterval == currRetryTimestamp)) {
                    updateRetryTO = true;
                }
#ifndef TS2_PKT_RETRY_ENABLE
                // delete acknowledged packet
                delete it->second->pkt;
#else
                // release data pointer for acknowledged packet
                it->second->data = NULL;
#endif
                // delete send_segment object
                delete it->second;
                map<u64, send_segment*>::iterator it_cur = it++;
                sendSegments.erase(it_cur);
            } else {
                // skip DATA packet
                LOG_DEBUG("skip segment["FMTu64"], it is a DATA packet", it->second->sendTimestamp + it->second->retryInterval);
                it++;
            }
        }
        if (updateRetryTO) {
            // wake the retry thread to update retry timestamp
            LOG_INFO("Update retry timestamp");
            VPLCond_Broadcast(&retry_cond);
        }
    }
}

int Ts2::Transport::sendWindow::CancelFromBuffers(u64 index)
{
    int rv = TS_OK;

    assert(VPLMutex_LockedSelf(&buffers_mutex));
    LOG_INFO("Cancel sending buffer["FMTu64"]", index);
    {
        // Remove all active segments from both sendSegments & retrySegments
        // and wake the retry thread to update the retry timestamp if necessary
        // Note: this nests the segs_mutex within the buffers_mutex.
        MutexAutoLock lock(&segs_mutex);
        bool updateRetryTO = false;
        map<u64, send_segment*>::iterator it = sendSegments.begin();
        // erase element from sendSegments
        while (it != sendSegments.end()) {
            if (it->second->head_seq_index == index) {
                // erase element from retrySegments
#ifndef TS2_PKT_RETRY_ENABLE
                LOG_DEBUG("erase segment["FMTu64"] of seg: "FMTu64" from retrySegments",
                    it->second->sendTimestamp + it->second->retryInterval,
                    it->second->pkt->GetSeqNum());
#else
                LOG_DEBUG("erase segment["FMTu64"] of seg: "FMTu64" from retrySegments",
                    it->second->sendTimestamp + it->second->retryInterval,
                    it->second->seqNum);
#endif
                retrySegments.erase(it->second->sendTimestamp + it->second->retryInterval);
                if (!updateRetryTO && (it->second->sendTimestamp + it->second->retryInterval == currRetryTimestamp)) {
                    updateRetryTO = true;
                }
#ifndef TS2_PKT_RETRY_ENABLE
                // delete acknowledged packet
                delete it->second->pkt;
#else
                // release data pointer for acknowledged packet
                it->second->data = NULL;
#endif
                // delete send_segment object
                delete it->second;
                map<u64, send_segment*>::iterator it_cur = it++;
                sendSegments.erase(it_cur);
            } else {
                LOG_DEBUG("skip segment["FMTu64"]", it->second->sendTimestamp + it->second->retryInterval);
                it++;
            }
        }
        if (updateRetryTO) {
            // wake the retry thread to update retry timestamp
            VPLCond_Broadcast(&retry_cond);
        }
    }
    {
        // Remove buffer from sendBuffers
        assert(VPLMutex_LockedSelf(&buffers_mutex));
        map<u64, send_buffer*>::iterator it = sendBuffers.find(index);
        if (it != sendBuffers.end()) {
            // delete send_buffer object
            delete it->second;
            sendBuffers.erase(index);
        }
        else {
            // Not found in sendBuffers. This implies the buffer was
            // acknowledged at the last second or a FIN came through
            // from the other side while the writer was waiting.
            // Actually this probably can't happen, since the buffer
            // mutex is held before calling this routine.
            rv = TS_ERR_INVALID;
        }
    }
    LOG_INFO("Cancellation is done, result: %d", rv);
    return rv;
}

int Ts2::Transport::sendWindow::InsertSegments(send_segment *segment)
{
    int rv = TS_OK;

    assert(VPLMutex_LockedSelf(&segs_mutex));

#ifndef TS2_PKT_RETRY_ENABLE
    sendSegments[segment->pkt->GetSeqNum()] = segment;
    sendSegments_size += segment->pkt->GetDataSize();
#else
    sendSegments[segment->seqNum] = segment;
    switch (segment->pktType) {
        case TS_PKT_DATA:
            // handling for DATA packet 
            LOG_DEBUG("seq: "FMTu64", data len: "FMTu32, segment->seqNum, segment->dataSize);
            sendSegments_size += segment->dataSize;
            break;
        default:
            // default packet handling
            break;
    }
#endif

    // Insert to retrySegments for retransmission
#ifndef TS2_PKT_RETRY_ENABLE
    LOG_DEBUG("insert retry at ["FMTu64"] for seg: "FMTu64" to retrySegments",
        segment->sendTimestamp + segment->retryInterval, segment->pkt->GetSeqNum());
#else
    LOG_DEBUG("insert retry at ["FMTu64"] for seg: "FMTu64" to retrySegments",
        segment->sendTimestamp + segment->retryInterval, segment->seqNum);
#endif
    retrySegments[segment->sendTimestamp + segment->retryInterval] = segment;
    // Wake the retry thread to update retry timestamp if necessary
    if (segment->sendTimestamp + segment->retryInterval < currRetryTimestamp) {
        VPLCond_Broadcast(&retry_cond);
    }
    LOG_DEBUG("segments count: "FMTu_size_t, sendSegments.size());
    LOG_DEBUG("retry count: "FMTu_size_t, retrySegments.size());

    return rv;
}

int Ts2::Transport::sendWindow::Clear(void)
{
    int rv = TS_OK;

    LOG_DEBUG("Clear sending buffers");
    {
        //
        // Mark sendBuffers aborted. Note that in this case, we keep the
        // buffer around and wake the thread in Vtunnel::Write, so that
        // it can recognize that the send was aborted and then delete the
        // send_buffer entry.
        //
        MutexAutoLock lock(&buffers_mutex);
        map<u64, send_buffer*>::iterator it = sendBuffers.begin();
        for ( ; it != sendBuffers.end(); it++) {
            LOG_DEBUG("Marking send buffer aborted, seq: "FMTu64, it->second->head_seq);
            it->second->sendAborted = true;
            // Wake up the writer and let it do the actual delete
            VPLCond_Signal(it->second->hComplete);
        }
    }
    {
        // Cancel retries and remove from sendSegments
        MutexAutoLock lock(&segs_mutex);
        bool deleted = false;
        map<u64, send_segment*>::iterator it = sendSegments.begin();
        while (it != sendSegments.end()) {
            // delete element in retrySegments
            retrySegments.erase(it->second->sendTimestamp + it->second->retryInterval);
            deleted = true;
#ifndef TS2_PKT_RETRY_ENABLE
            // delete acknowledged packet
            delete it->second->pkt;
#else
            // release data pointer for acknowledged packet
            it->second->data = NULL;
#endif
            // delete send_segment object
            delete it->second;
            map<u64, send_segment*>::iterator it_cur = it++;
            sendSegments.erase(it_cur);
        }
        if (deleted) {
            // wake the retry thread to update timer
            VPLCond_Broadcast(&retry_cond);
        }
    }
    LOG_DEBUG("Sending buffers cleared, result: %d", rv);
    return rv;
}

// Class method (declared static in the header)
VPLTHREAD_FN_DECL Ts2::Transport::Vtunnel::retryThread(void* arg)
{
    Ts2::Transport::Vtunnel *vt = (Ts2::Transport::Vtunnel*)arg;

    vt->retryThread();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

void Ts2::Transport::Vtunnel::retryThread()
{
    int rv = VPL_OK;

    LOG_DEBUG("retry thread begins: lcl vt "FMTu32, lcl_vt_id);

    VPLMutex_Lock(&sendWin->segs_mutex);

    do {
        if (rv == VPL_OK) {
            LOG_DEBUG("retry thread is awakened");
            // Retry_cond is signaled or it is the very first time in the loop
            // Update currRetryTimestamp from retrySegments
            map<VPLTime_t, send_segment*>::iterator it = sendWin->retrySegments.begin();
            if (it == sendWin->retrySegments.end()) {
                LOG_DEBUG("no pkt to retransmit, so update retry timestamp from "FMTu64" to MAX", sendWin->currRetryTimestamp);
                sendWin->currRetryTimestamp = VPL_TIMEOUT_NONE;
            }
            else {
                sendWin->currRetryTimestamp = it->first;
                LOG_DEBUG("update retry timestamp to "FMTu64, sendWin->currRetryTimestamp);
            }
        }
        else if (rv == VPL_ERR_TIMEOUT) {
            LOG_DEBUG("retry thread is awakened by timeout");
            // Timeout, retransmit every segment still in the map which has retry timestamp <= currRetryTimestamp
            map<VPLTime_t, send_segment*>::iterator it = sendWin->retrySegments.begin();
            if (it == sendWin->retrySegments.end()) {
                LOG_DEBUG("Timeout but no pkt to retransmit, still update retry timestamp from "FMTu64" to MAX", sendWin->currRetryTimestamp);
                sendWin->currRetryTimestamp = VPL_TIMEOUT_NONE;
            }
            else {
                do {
                    if (it->first > sendWin->currRetryTimestamp) {
                        // update currRetryTimestamp nearest expired timestamp
                        LOG_DEBUG("update retry timestamp from "FMTu64" to "FMTu64, sendWin->currRetryTimestamp, it->first);
                        sendWin->currRetryTimestamp = it->first;
                        break;
                    }
                    else {
#ifndef TS2_PKT_RETRY_ENABLE
                    LOG_WARN("ack[%p], wait result: %d, time to retry seq: "FMTu64,
                        &it->second,
                        rv,
                        it->second->pkt->GetSeqNum());
#else
                    LOG_WARN("ack[%p], wait result: %d, time to retry seq: "FMTu64,
                        &it->second,
                        rv,
                        it->second->seqNum);
#endif
                        // Packet retransmission
#ifndef TS2_PKT_RETRY_ENABLE
                        // duplicate packet for retransmission
                        PacketData *origPkt = it->second->pkt;
                        PacketData *sndPkt = new (std::nothrow) PacketData(
                                                            origPkt->GetSrcDeviceId(), origPkt->GetSrcInstId(), origPkt->GetSrcVtId(),
                                                            origPkt->GetDstDeviceId(), origPkt->GetDstInstId(), origPkt->GetDstVtId(),
                                                            origPkt->GetSeqNum(),
                                                            origPkt->GetAckNum(),
                                                            origPkt->GetData(),
                                                            origPkt->GetDataSize(),
                                                            recvWin->window_capacity - recvWin->recvSegments_size,
                                                            false);
                        LOG_WARN("retry sending packet lcl vt "FMTu32", seq: "FMTu64", len: "FMTu32,
                            lcl_vt_id, it->second->pkt->GetSeqNum(), it->second->pkt->GetDataSize());
#else
                        Packet *sndPkt = NULL;
                        // create packet for retransmission only when needed
                        switch (it->second->pktType) {
                        case TS_PKT_SYN:
                            {
                                LOG_WARN("retry sending SYN packet lcl vt "FMTu32", seq: "FMTu64", svcName: %s",
                                    lcl_vt_id, it->second->seqNum, it->second->svcName.c_str());
                                PacketSyn *Pkt = new (std::nothrow) Ts2::PacketSyn(
                                                                src_udi.deviceId, src_udi.instanceId, lcl_vt_id,
                                                                dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id,
                                                                it->second->seqNum,
                                                                recvWin->cur_ack,  // it->second->ackNum,
                                                                it->second->svcName);
                                sndPkt = (Packet*)Pkt;
                                Pkt = NULL;
                            }
                            break;
                        case TS_PKT_SYN_ACK:
                            {
                                LOG_WARN("retry sending SYN ACK packet lcl vt "FMTu32", seq: "FMTu64,
                                    lcl_vt_id, it->second->seqNum);
                                PacketSynAck *Pkt = new (std::nothrow) Ts2::PacketSynAck(
                                                                   src_udi.deviceId, src_udi.instanceId, lcl_vt_id,
                                                                   dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id,
                                                                   it->second->seqNum,
                                                                   recvWin->cur_ack,  // it->second->ackNum,
                                                                   recvWin->window_capacity - recvWin->recvSegments_size);
                                sndPkt = (Packet*)Pkt;
                                Pkt = NULL;
                            }
                            break;
                        case TS_PKT_DATA:
                            {
                                LOG_WARN("retry sending DATA packet lcl vt "FMTu32", seq: "FMTu64", len: "FMTu32,
                                    lcl_vt_id, it->second->seqNum, it->second->dataSize);
                                PacketData *Pkt = new (std::nothrow) Ts2::PacketData(
                                                                 src_udi.deviceId, src_udi.instanceId, lcl_vt_id,
                                                                 dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id,
                                                                 it->second->seqNum,
                                                                 recvWin->cur_ack,  // it->second->ackNum,
                                                                 it->second->data,
                                                                 it->second->dataSize,
                                                                 recvWin->window_capacity - recvWin->recvSegments_size,
                                                                 false);
                                sndPkt = (Packet*)Pkt;
                                Pkt = NULL;
                            }
                            break;
                        case TS_PKT_FIN:
                            {
                                LOG_WARN("retry sending FIN packet lcl vt "FMTu32", seq: "FMTu64,
                                    lcl_vt_id, it->second->seqNum);
                                PacketFin *Pkt = new (std::nothrow) Ts2::PacketFin(
                                                                src_udi.deviceId, src_udi.instanceId, lcl_vt_id,
                                                                dst_udi.deviceId, dst_udi.instanceId, rmt_vt_id,
                                                                it->second->seqNum,
                                                                recvWin->cur_ack);
                                sndPkt = (Packet*)Pkt;
                                Pkt = NULL;
                            }
                            break;
                        default:
                            LOG_WARN("Unknown type of packet to retransmitted: "FMTu8, it->second->pktType);
                            break;
                        }
#endif
                        if (sndPkt == NULL) {
                            // Probably can't continue in this case
                            LOG_ERROR("Allocation failure!");
                            break;
                        }

                        // Update send_segment object in retrySegments
                        VPLTime_t prevTimestamp = it->second->sendTimestamp + it->second->retryInterval;
#ifdef TS2_ENABLE_RTT_BASED_RETRY
                        // update retry interval for exponential back-off
                        // upper bound is defined by ts2MaxRetransmitIntervalMs
                        it->second->retryRound = 
                            (it->second->retryRound * 2 < localInfo->GetMaxRetransmitRound()) ? (it->second->retryRound * 2) : localInfo->GetMaxRetransmitRound();
                        it->second->retryInterval = it->second->retryRound * sendWin->GetCurrentDefaultRetryInterval();
                        LOG_WARN("retry[%d] current default interval: "FMTu64" ms, interval is "FMTu64" ms now", 
                            it->second->retryRound,
                            VPLTime_ToMillisec(sendWin->GetCurrentDefaultRetryInterval()),
                            VPLTime_ToMillisec(it->second->retryInterval));
#else
                        it->second->retryInterval = sendWin->GetCurrentDefaultRetryInterval();
#endif

                        // update sendTimestamp for retry
                        it->second->sendTimestamp = VPLTime_GetTimeStamp();
#ifndef TS2_PKT_RETRY_ENABLE
                        LOG_DEBUG("update segment timestamp["FMTu64"] to ["FMTu64"] for seqNum "FMTu64,
                            prevTimestamp,
                            it->second->sendTimestamp + it->second->retryInterval,
                            it->second->pkt->GetSeqNum());
#else
                        LOG_DEBUG("update segment timestamp["FMTu64"] to ["FMTu64"] for seqNum "FMTu64,
                            prevTimestamp,
                            it->second->sendTimestamp + it->second->retryInterval,
                            it->second->seqNum);
#endif
                        // Check that the "new" succeeded
                        if (sndPkt != NULL) {
                            // enqueue segment again for retransmission
                            // give its retryInterval as packet TTL
                            sendPacket(sndPkt, it->second->retryInterval);
                            // hand over sndPkt to send window
                            sndPkt = NULL;
                        }

                        // insert the send_segment object with latest retry timestamp
                        sendWin->retrySegments[it->second->sendTimestamp + it->second->retryInterval] = it->second;
                        // erase the one with previous retry timestamp from retrySegments
                        it++;
                        sendWin->retrySegments.erase(prevTimestamp);
                    }
                }  while (it != sendWin->retrySegments.end());
            }
        }
        else {
            LOG_DEBUG("retry thread is awakened by error: %d", rv);
        }

        VPLTime_t nextTO = ::VPLTime_DiffClamp(sendWin->currRetryTimestamp, VPLTime_GetTimeStamp());
        if (nextTO == 0) {
            // Current timestamp is larger than currRetryTimestamp, so it's time to do retransmission
            rv = VPL_ERR_TIMEOUT;
        }
        else {
            if (isRetryStop) break;
            LOG_DEBUG("Sleeping "FMTu64" ms for retransmission", VPLTime_ToMillisec(nextTO));
            rv = VPLCond_TimedWait(&sendWin->retry_cond, &sendWin->segs_mutex, nextTO);
        }

        if (isRetryStop) {
            break;
        }
    } while (!isRetryStop);

    VPLMutex_Unlock(&sendWin->segs_mutex);

    LOG_INFO("retry thread exits: lcl vt "FMTu32, lcl_vt_id);
    return;
}

Ts2::Transport::recvWindow::recvWindow(int maxWindowSize) :
    window_capacity(maxWindowSize),
    recvSegments_size(0),
    head_seq(0),
    cur_ack(0),
    recvThreadCount(0),
    closeInProgress(false)
{
    VPLMutex_Init(&segs_mutex);
    VPLCond_Init(&recv_cond);
    recvSegments.clear();
}

Ts2::Transport::recvWindow::~recvWindow()
{
    map<u64, PacketData*>::iterator it;

    VPLMutex_Lock(&segs_mutex);
    // Free any packets remaining on the receive queue
    while (1) {
        it = recvSegments.begin();
        if (it == recvSegments.end()) {
            break;
        }
        delete it->second;
        recvSegments.erase(it);
    }
    VPLMutex_Unlock(&segs_mutex);
    VPLMutex_Destroy(&segs_mutex);
    VPLCond_Destroy(&recv_cond);
}
