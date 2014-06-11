//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "EventManagerPb.hpp"

#include "vpl_th.h"

#include "AutoLocks.hpp"

#include <vector>

/// The ring buffer size; the actual maximum number of events is one less.
#define EVENT_MANAGER_PB_MAX_EVENTS  1024

static bool s_isInit = false;
static bool s_isRunning = false;

/// Whenever the mutex is released, all of the following must be true:
/// @invariant All elements of s_eventRing.buffer before popIndex and at or after pushIndex are NULL.
/// @invariant s_eventRing.popIndex == the currPos of the EventQueue with the oldest unread event.
///     or equivalently:
///     s_eventRing.numQueued() == MAX(x.numQueued() for all EventQueue x in s_queues)

static VPLMutex_t s_mutex;

static VPLCond_t s_condVar;

static u64 s_nextHandle = 1;

static int s_numThreadsWaiting = 0;

#define NEXT_RING_IDX(x_)  (((x_) + 1) % EVENT_MANAGER_PB_MAX_EVENTS)

struct EventRing
{
    /// Points to oldest still-valid event.
    /// When pushIndex == popIndex, the buffer should be considered empty.
    int popIndex;

    /// Points to the next free spot.
    int pushIndex;

    /// Ring buffer of pointers to events.
    /// EventRing is responsible for calling delete on each element as it is removed from the buffer.
    ccd::CcdiEvent* buffer[EVENT_MANAGER_PB_MAX_EVENTS];

    EventRing() : popIndex(0), pushIndex(0) {
        memset(buffer, 0, ARRAY_SIZE_IN_BYTES(buffer));
    }

    int numQueued() const
    {
        return ((pushIndex + EVENT_MANAGER_PB_MAX_EVENTS) - popIndex) % EVENT_MANAGER_PB_MAX_EVENTS;
    }

    void enqueue(const ccd::CcdiEvent* event)
    {
        buffer[pushIndex] = const_cast<ccd::CcdiEvent*>(event);

        pushIndex = NEXT_RING_IDX(pushIndex);

        // When pushIndex == popIndex, the buffer is considered empty.
        // We should have cleared out any old queues before calling this.
        ASSERT_NOT_EQUAL(pushIndex, popIndex, "%d");
    }

    void dequeue()
    {
        // Make sure that the buffer is not already empty.
        ASSERT_NOT_EQUAL(pushIndex, popIndex, "%d");
        delete buffer[popIndex];
        buffer[popIndex] = NULL;
        popIndex = NEXT_RING_IDX(popIndex);
    }

    void trimTo(int count)
    {
        LOG_INFO("Deleting %d event(s)", (numQueued() - count));
        while (numQueued() > count) {
            dequeue();
        }
    }

    void clear()
    {
        trimTo(0);
    }
};

static EventRing s_eventRing;

struct EventQueue {
    u64 handle;
    /// Next index to return.
    int currPos;

    int numQueued() const {
        return ((s_eventRing.pushIndex + EVENT_MANAGER_PB_MAX_EVENTS) - currPos) % EVENT_MANAGER_PB_MAX_EVENTS;
    }

    const ccd::CcdiEvent& peek() const {
        return *(s_eventRing.buffer[currPos]);
    }

    void advance() {
        currPos = NEXT_RING_IDX(currPos);
    }

    bool isOldest() const {
        return (currPos == s_eventRing.popIndex);
    }
};

static std::vector<EventQueue> s_queues;

int EventManagerPb_Init()
{
    int rv = 0;
    if (!s_isInit) {
        rv = VPLMutex_Init(&s_mutex);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "VPLMutex_Init", rv);
            goto out;
        }
        rv = VPLCond_Init(&s_condVar);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "VPLCond_Init", rv);
            goto out;
        }
        // Bug 6905: Select the starting queue handle based on the current time.
        // This is important to avoid the race condition where CCD is stopped and restarted
        // and a queue handle number gets recycled to a new client without the old client calling
        // EventsDequeue before the queue is recreated.
        // To make it easier to read, we want the starting handle to end with "0001" when written in base 10.
        // Chop off the top 14 bits to prevent the multiplication from overflowing (2^14 = 16384).
        s_nextHandle = (((VPLTime_GetTime() << 14) >> 14) * 10000) + 1;
        LOG_INFO("First queue handle is "FMTu64, s_nextHandle);
        s_isInit = true;
    }
    s_isRunning = true;
 out:
    return rv;
}

static void
wakeBlockedThreads()
{
    ASSERT(VPLMutex_LockedSelf(&s_mutex));

    // Wake up any blocked threads.
    LOG_DEBUG("Waking %d thread(s)", s_numThreadsWaiting);
    VPLCond_Broadcast(&s_condVar);
    s_numThreadsWaiting = 0;
}

void EventManagerPb_Quit()
{
    {
        MutexAutoLock lock(&s_mutex);

        wakeBlockedThreads();

        s_queues.clear();
        s_eventRing.clear();
        s_isRunning = false;
    }
}

/// Delete any events that are no longer accessible.
static void
privCleanup()
{
    ASSERT(VPLMutex_LockedSelf(&s_mutex));

    int largestQueue = 0;
    for (int i = 0; i < s_queues.size(); i++) {
        int currSize = s_queues[i].numQueued();
        if (currSize > largestQueue) {
            largestQueue = currSize;
        }
    }
    LOG_INFO("Largest remaining queue size: %d", largestQueue);
    s_eventRing.trimTo(largestQueue);
}

void EventManagerPb_AddEvent(const ccd::CcdiEvent* event)
{
    LOG_INFO("Enqueue CcdiEvent: %s", event->ShortDebugString().c_str());
    
    MutexAutoLock lock(&s_mutex);
    if (!s_isRunning) {
        LOG_WARN("EventManagerPb is not running; dropping event");
        delete event;
        return;
    }
    if (s_queues.size() == 0) {
        LOG_INFO("No active queues; dropping event");
        delete event;
        return;
    }

    // Check if this will cause any existing queues to reach capacity.
    {
        bool doCleanup = false;
        int full = (s_eventRing.pushIndex + 1) % EVENT_MANAGER_PB_MAX_EVENTS;
        for (int i = 0; i < s_queues.size(); i++) {
            if (s_queues[i].currPos == full) {
                LOG_WARN("Event queue "FMTu64" has become full; deleting it.", s_queues[i].handle);
                doCleanup = true;
                // Replace the current element with the last one and shrink the vector by 1.
                if (i < s_queues.size() - 1) {
                    s_queues[i] = s_queues[s_queues.size() - 1];
                }
                s_queues.pop_back();
                i--; // stay on the same index for the next iteration.
            }
        }
        if (doCleanup) {
            // We deleted at least 1 queue; remove any events that are no longer needed.
            privCleanup();
        }
    }

    // Enqueue the event.
    s_eventRing.enqueue(event);

    // Wake up any blocked threads.
    wakeBlockedThreads();
}

int EventManagerPb_CreateQueue(u64* newHandle_out)
{
    MutexAutoLock lock(&s_mutex);
    if (!s_isRunning) {
        return CCD_ERROR_NOT_RUNNING;
    }

    LOG_INFO("Creating event queue "FMTu64, s_nextHandle);
    EventQueue newQueue = { s_nextHandle, s_eventRing.pushIndex };
    s_queues.push_back(newQueue);
    *newHandle_out = s_nextHandle;
    s_nextHandle++;

    return 0;
}

int EventManagerPb_DestroyQueue(u64 handle)
{
    MutexAutoLock lock(&s_mutex);
    if (!s_isRunning) {
        return CCD_ERROR_NOT_RUNNING;
    }

    for (int i = 0; i < s_queues.size(); i++) {
        if (s_queues[i].handle == handle) {
            LOG_INFO("Destroying event queue "FMTu64, handle);

            bool doCleanup = s_queues[i].isOldest();

            // Replace the current element with the last one and shrink the vector by 1.
            if (i < s_queues.size() - 1) {
                s_queues[i] = s_queues[s_queues.size() - 1];
            }
            s_queues.pop_back();

            if (doCleanup) {
                privCleanup();
            }

            // Wake up any blocked threads.
            wakeBlockedThreads();

            return 0;
        }
    }
    // Not found; it may have already been deleted.
    return CCD_ERROR_NO_QUEUE;
}

static EventQueue*
findQueue(u64 queueHandle)
{
    ASSERT(VPLMutex_LockedSelf(&s_mutex));

    for (int i = 0; i < s_queues.size(); i++) {
        EventQueue* queue = &s_queues[i];
        if (queue->handle == queueHandle) {
            return queue;
        }
    }
    return NULL;
}

int EventManagerPb_GetEvents(u64 queueHandle, u32 maxToGet, int timeoutMs,
        google::protobuf::RepeatedPtrField<ccd::CcdiEvent>* events_out)
{
    MutexAutoLock lock(&s_mutex);
    if (!s_isRunning) {
        return CCD_ERROR_NOT_RUNNING;
    }
    if (timeoutMs < -1) {
        LOG_ERROR("Invalid timeout value");
        return CCD_ERROR_PARAMETER;
    }
    VPLTime_t timeoutTime = VPLTime_GetTimeStamp() + VPLTIME_FROM_MILLISEC(timeoutMs);

start_again:
    EventQueue* queue = findQueue(queueHandle);
    if (queue == NULL) {
        // Not found.
        LOG_INFO("No queue found for handle "FMTu64, queueHandle);
        return CCD_ERROR_NO_QUEUE;
    } else {
        // Found the queue.
        if (queue->numQueued() > 0) {
            bool doCleanup = false;
            if (queue->isOldest()) {
                // Any events that we return may potentially need to be deleted.
                doCleanup = true;
            }
            // 0 indicates no limit.
            if (maxToGet == 0) {
                maxToGet = EVENT_MANAGER_PB_MAX_EVENTS;
            }
            while (maxToGet > 0) {
                ccd::CcdiEvent* newEvent = events_out->Add();
                *newEvent = queue->peek();
                maxToGet--;
                queue->advance();
                if (queue->numQueued() == 0) {
                    // No more events.
                    break;
                }
            }
            if (doCleanup) {
                privCleanup();
            }
            return 0;
        } else {
            LOG_DEBUG("No events queued for handle "FMTu64, queueHandle);
            VPLTime_t remaining;
            if (timeoutMs == -1) {
                remaining = VPL_TIMEOUT_NONE;
            } else {
                remaining = VPLTime_DiffClamp(timeoutTime, VPLTime_GetTimeStamp());
            }
            if (remaining == 0) {
                LOG_DEBUG("Timeout (%dms) was reached; no new events", timeoutMs);
                return 0;
            }
            // Block until timeout or until signaled.
            // NOTE: Once we release the mutex by calling VPLCond_TimedWait, "EventQueue* queue" is no longer safe to use.
            s_numThreadsWaiting++;
            LOG_DEBUG("Waiting for event to dequeue; remaining="FMT_VPLTime_t, remaining);
            int temp_rv = VPLCond_TimedWait(&s_condVar, &s_mutex, remaining);
            if ((temp_rv != VPL_OK) && (temp_rv != VPL_ERR_TIMEOUT)) {
                LOG_ERROR("%s failed: %d", "VPLCond_TimedWait", temp_rv);
                return CCD_ERROR_INTERNAL;
            }
            if (!s_isRunning) {
                LOG_INFO("Module was shut down while waiting; no new events");
                return CCD_ERROR_NOT_RUNNING;
            }
            goto start_again;
        }
    }
}
