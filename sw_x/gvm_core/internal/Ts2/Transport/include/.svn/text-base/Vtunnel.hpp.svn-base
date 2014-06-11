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

///
/// Vtunnel.hpp
///
/// Class Definition for a TS Vtunnel Connection
///
#ifndef __VTUNNEL_HPP__
#define __VTUNNEL_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include "ts_internal.hpp"
#include "FrontEnd.hpp"
#include "Packet.hpp"

// TODO: TS enable pkt retransmission when ready
#define TS2_PKT_RETRY_ENABLE
#define TS2_ENABLE_RTT_BASED_RETRY

namespace Ts2 {
namespace Transport {

// sending window and segments
struct send_segment {
#ifndef TS2_PKT_RETRY_ENABLE
    PacketData* pkt;     // packet waiting for ack
#else
    u8 pktType;
    u64 seqNum;
    u64 ackNum;
    string svcName;
    const u8* data;
    u32 dataSize;
#endif
    VPLTime_t sendTimestamp;  // sending timestamp of the packet
    int retryRound;
    VPLTime_t retryInterval;  // value of current retry interval
    u64 head_seq_index;  // index of head_seq in sendBuffers
    VPLSem_t hAcked;     // semaphore to signal when the segment is acknowledged
};

struct send_buffer {
    const void* buffer;   // the buffer to send
    size_t buffer_size;   // size of the buffer to send
    u64 head_seq;         // the first seq number of the buffer
    VPLCond_t* hComplete; // the condition variable to signal when the buffer is sent and acknowledged
    bool sendAborted;     // set when a FIN interrupts a write in progress
};

class sendWindow {
    // sending window
public:
    sendWindow(int maxSegmentSize, int maxWindowSize, VPLTime_t retryInterval);
    ~sendWindow();

    inline void SetWindowSize(u64 window_size) { window_capacity = window_size; };
    u64 GetWindowSize(void) const { return window_capacity; };
    size_t GetSegmentSize(void) const { return segment_size; };
    bool isAvailableWindow(void) const { return (window_capacity - sendSegments_size) >= segment_size; };
    bool isAvailableBufferToSend(void) const { return nextBufferIndex > (currBufferIndex + currBufferOffset); };
    int AppendToBuffers(const void* buffer, size_t buffer_size, VPLCond_t* complete_cond, u64& index);
    int CancelFromBuffers(u64 index);    // Cancel buffer and every segments in sendSegments & retrySegments with specific head seq index
    void CancelCtrlPktFromSegments();  // Cancel ctrl pkt segments in sendSegments and retrySegments with specific head seq index
    int InsertSegments(send_segment *segment);
    int Clear(void);
    VPLTime_t GetCurrentDefaultRetryInterval() { 
#ifdef TS2_ENABLE_RTT_BASED_RETRY
        return 2*currRTT;
#else
        return currRTT;
#endif
    };

    map<u64, send_segment*> sendSegments;   // list of segments which are sent but waiting for being acknowledged, key is seq number
    map<VPLTime_t, send_segment*> retrySegments;  // list of segments which are to be retransitted later, key is retry timestamp
    map<u64, send_buffer*> sendBuffers;     // buffers to send from caller, key is send_buffer::head_seq
    u64 sendSegments_size; // sending segments total size, must be less than _UI64_MAX
    u64 sendBuffers_size;  // sending buffers total size, must be less than _UI64_MAX
    u64 window_capacity;   // window capacity, can be adaptive to receiver's window and network congestion
    size_t segment_size;   // segmentation size, can be adaptive to network congestion
    u64 cur_seq;           // the current ack number

    u64 currBufferIndex;   // the index of the current buffer to send
    u64 currBufferOffset;  // the offset within the current buffer being sent
    u64 nextBufferIndex;   // the next buffer index to append

    VPLMutex_t segs_mutex;         // protects sendSegments and retrySegments (active sending window)
    VPLMutex_t buffers_mutex;      // protects sendBuffers (application write buffers)
    VPLCond_t seg_cond;
    VPLCond_t buffer_cond;
    VPLCond_t retry_cond;
    VPLTime_t currRTT;             // current round trip time
    VPLTime_t currRetryTimestamp;  // current expired time for next retransmission in retry thread
};

// receiving window and segments
class recvWindow {
    // receiving window
public:
    recvWindow(int maxWindowSize);
    ~recvWindow();

    map<u64, PacketData*> recvSegments; // list of segments received
    u64 window_capacity;     // window capacity, can be adaptive to receiver's window and network congestion
    u64 recvSegments_size;   // receive buffers total size, must be less than _UI64_MAX
    u64 head_seq;            // head sequence number of the unread segments
    u64 cur_ack;             // ack number already replied

    int recvThreadCount;     // current threads waiting to read
    bool closeInProgress;    // used to release readers at close time

    VPLMutex_t segs_mutex;
    VPLCond_t recv_cond;
};

class Vtunnel {
public:
    Vtunnel(const ts_udi_t& src_udi, const ts_udi_t& dst_udi, const string& svcName, LocalInfo *localInfo);
    Vtunnel(const ts_udi_t& src_udi, const ts_udi_t& dst_udi, u64 dstStartSeqNum, u32 dst_vt_id, const string& svcName, LocalInfo *localInfo);
    ~Vtunnel();
   
    int Open(string& err_msg);
    int Read(void* buffer, size_t& len, string& err_msg);
    int Write(const void* buffer, size_t len, string& err_msg);
    int Close(string& err_msg);

    int HandleSyn(Packet* pkt, s32 error);    // called by Ts2 server side to handle syn packet
    void ReceivePacket(Packet* pkt);

    int  WaitForOpen();
    int  WaitForClose();
    int  ForceClose();
    void SetVtId(u32 lcl_vt_id);
    u32  GetVtId();
    u32  GetDstVtId();
    u64  GetDstDeviceId();
    u32  GetDstInstanceId();
    u64  GetDstStartSeqNum();

    void SetFrontEnd(Ts2::Network::FrontEnd* frontEnd);
    bool IsConnectingDevice(u64 deviceId);    // called by Pool to tell whether src or dst is specific deviceId

    int AddRefCount(int delta);
    int GetRefCount();

private:
    VPLMutex_t mutex;
    VPLCond_t open_cond;
    VPLCond_t close_cond;
    VPLCond_t clients_cond;

    enum VtunnelState {
        VT_STATE_NULL = 0,
        VT_STATE_OPENING,    // Client side: SYN sent
        VT_STATE_HALF_OPEN,  // Server side: SYN_ACK sent
        VT_STATE_OPEN,
        VT_STATE_CLOSING,    // FIN sent or received
        VT_STATE_CLOSE_WAIT, // Server side waiting for FIN or FIN_ACK
        VT_STATE_CLOSED,     // FIN_ACK received
    };
    VtunnelState state;
    int activeClients;
    Ts2::Network::FrontEnd* networkFrontEnd;

    // Private methods
    static VPLTHREAD_FN_DECL writerThread(void* arg);
    void writerThread();
    void stopWorkerThreads();
    static VPLTHREAD_FN_DECL retryThread(void* arg);
    void retryThread();
    void closeTunnel(bool sendFin);

    // Packet handlers
    void handleAck(Packet* pkt);
    void handleData(Packet* pkt);
    void handleThreeWayAck(Packet* pkt, bool bDeletePkt=true);
    void handleFin(Packet* pkt);
    void handleFinAck(Packet* pkt);
    void sendPacket(Packet* pkt, VPLTime_t pktTTL);
    void reSendAck(u64 ackNo);

    sendWindow* sendWin;
    recvWindow* recvWin;
    list<Packet*> openQueue;     // open packets to be handled
    ts_udi_t src_udi;            // local udi
    ts_udi_t dst_udi;            // remote udi
    u64 dstStartSeqNum;          // remote start sequence number, implies difference TS connection attempt
    const string& svcName;       // service name requested
    u32 lcl_vt_id, rmt_vt_id;    // vtunnel id of local and remote devices

    VPLDetachableThreadHandle_t writerHandle;
    bool isWriterStarted;        // writer thread is running
    bool isWriterStop;           // flag to stop writer thread

    VPLDetachableThreadHandle_t retryHandle;  // the thread for retransmission
    bool isRetryThreadStarted;   // retry thread is running
    bool isRetryStop;            // flag to stop retry thread

    LocalInfo *localInfo;
};

} // end namespace Transport
} // end namespace Ts2

#endif // include guard
