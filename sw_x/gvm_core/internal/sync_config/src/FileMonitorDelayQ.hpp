//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//


#ifndef FILE_MONITOR_DELAY_Q_HPP_3_11_2013
#define FILE_MONITOR_DELAY_Q_HPP_3_11_2013

#include "vpl_thread.h"
#include "vpl_time.h"
#include "vpl_fs.h"
#include "vpl_th.h"
#include <string>
#include <vector>
#include <list>
#include <deque>

/// Callback to be passed to VPLFS_MonitorDir.
/// @param[in] handle Handle that generated the event
/// @param[in] eventBuffer Buffer containing zero or more events.  The event buffer
///                        is owned by the VPL library, and once the callback is
///                        complete, the event buffer must not be modified by the
///                        user.
/// @param[in] eventBufferSize Size of buffer returned
/// @param[in] error Error code
///                  VPLFS_MonitorCB_OK
///                  VPLFS_MonitorCB_OVERFLOW buffer not large enough, events lost
///                  VPLFS_MonitorCB_UNMOUNT
/// @param[in] ctx Opaque callback context that will be passed back in callback
typedef void (*FileMonitorDelayQCallback)(VPLFS_MonitorHandle handle,
                                          void* eventBuffer,
                                          int eventBufferSize,
                                          int error,
                                          void* ctx);

struct DelayQEntry
{
    VPLTime_t timeQueued;

    VPLFS_MonitorHandle handle;
    void* eventBuffer;
    int eventBufferSize;
    int error;

    // Needed for linux, event buffer does not contain the string, only ptr.
    std::list<std::string>* stringPool;

    DelayQEntry()
    :  timeQueued(0),
       eventBuffer(NULL),   // Malloc and free used on this field
       eventBufferSize(0),
       error(0),
       stringPool(NULL)
    {}
};

struct FileMonitorHandleMap
{
    VPLFS_MonitorHandle handle;
    FileMonitorDelayQCallback cb;
    void* cb_ctx;
};

class FileMonitorDelayQ
{
public:
    FileMonitorDelayQ(VPLTime_t delayAmount);
    ~FileMonitorDelayQ();

    int Init();
    int Shutdown();

    int AddMonitor(const std::string& directory,
                   int num_events_internal,
                   FileMonitorDelayQCallback cb,
                   void* context,
                   VPLFS_MonitorHandle* handle_out);
    int RemoveMonitor(VPLFS_MonitorHandle handle);

private:
    u32 m_initCount;
    bool m_threadRunning;
    int removeMonitor(VPLFS_MonitorHandle handle);
    // Allow the FileMonitor static callback function to enqueue events
    // into the delayer
    static void fileMonitorCallback(VPLFS_MonitorHandle handle,
                                    void* eventBuffer,
                                    int eventBufferSize,
                                    int error);
    static void AddFileMonitorDelayQueue(FileMonitorDelayQ* delayQ);
    static void RemoveFileMonitorDelayQueue(FileMonitorDelayQ* delayQ);
    static VPLTHREAD_FN_DECL FileMonitorDelayQ_ThreadFn(void* arg);
    bool GetHandleMap(VPLFS_MonitorHandle handle,
                      FileMonitorHandleMap& handleMap_out);
    //void* GetContext(VPLFS_MonitorHandle handle);
    void EnqueueEvent(VPLFS_MonitorHandle handle,
                      void* eventBuffer,
                      int eventBufferSize,
                      int error);
    void delayQLoop();
    bool hasWork(VPLTime_t currTime);

    VPLTime_t m_delayAmount;

    VPLMutex_t m_api_mutex; // Prevents destructor from yanking away object.

    VPLDetachableThreadHandle_t m_delayQ_thread;
    VPLMutex_t m_delayQ_mutex; // Protects m_delayQ and m_handleMap
    VPLCond_t m_delayQ_cond_var;
    std::deque<DelayQEntry> m_delayQ;  // push from back, pop from front.
    std::vector<FileMonitorHandleMap> m_handleMap;  // protected by m_delayQ_mutex
};

#endif // FILE_MONITOR_DELAY_Q_HPP_3_11_2013
