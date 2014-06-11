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
#include "vsTest_worker.h"

#include "vpl_lazy_init.h"
#include "vpl_time.h"
#include "vpl_th.h"
#include "vplex_trace.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define DEBUG_WORK_QUEUE 0

typedef struct vsTest_worker_task {
    /// Function for this task
    void (*task)(void* params);

    /// Parameters for the task function
    void* params;

#if DEBUG_WORK_QUEUE
    int task_id;
    int tasks_ahead;
    VPLTime_t add_time;
    VPLTime_t run_time;
    VPLTime_t done_time;
#endif
} vsTest_worker_task;

// Worker task queue, synchronization objects, and thread.
static int vsTest_q_head = 0; // Index of first task in queue, if any
static int vsTest_q_tail = 0; // Index of first open slot, if any
static int vsTest_q_size = 0; // number of tasks in queue
static int vsTest_q_cap = 0;  // capacity of queue (working size)
static int vsTest_q_max = 0; // high-watermark for queue usage, for debugging.
#define VSTEST_Q_HWM_OBSERVED 2000 // deepest task queue observed
#define VSTEST_Q_INITIAL_CAP ((((VSTEST_Q_HWM_OBSERVED * sizeof(vsTest_worker_task)) + 4095) & ~4095) / sizeof(vsTest_worker_task))
#define VSTEST_Q_GROW_QTY  (4096 / sizeof(vsTest_worker_task)) // one page worth, about 512 entries non-debug, 102 debug
static vsTest_worker_task* vsTest_task_q = NULL; // pointer to queue when allocated.
static VPLLazyInitMutex_t vsTest_worker_mutex = VPLLAZYINITMUTEX_INIT;
static VPLCond_t vsTest_worker_condvar = VPLCOND_SET_UNDEF;

// Size of worker stack
#define WORKER_STACK_SIZE  (32*1024)
static VPLThread_t* vsTest_worker_threads = NULL;
static int num_threads = 0;
static int num_threads_running = 0;
static volatile int vsTest_worker_run = 0;

/// Actual worker thread function.
static void* vsTest_worker(void* unused)
{
    vsTest_worker_task work_unit;

    // While allowed to run, wait for tasks.
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex));
    while(vsTest_worker_run) {
        if(vsTest_q_size == 0) {
            // Check needed in the case a task is added before this main loop is entered,
            // in which the signal is missed and the condvar sleeps forever...
            VPLCond_TimedWait(&vsTest_worker_condvar, VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex), VPL_TIMEOUT_NONE);
        }

        if(vsTest_worker_run && vsTest_q_size > 0) {
            memcpy(&work_unit, &vsTest_task_q[vsTest_q_head],
                   sizeof(vsTest_worker_task));
            vsTest_q_head = (vsTest_q_head + 1) % vsTest_q_cap;
            vsTest_q_size--;

            VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex));

            VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                "Processing task. Function %p, context %p.",
                                work_unit.task, work_unit.params);
#if DEBUG_WORK_QUEUE
            work_unit.run_time = VPLTime_GetTimeStamp();
#endif
            work_unit.task(work_unit.params);

            VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex));

#if DEBUG_WORK_QUEUE
            work_unit.done_time = VPLTime_GetTimeStamp();

            if(VPLTime_DiffAbs(work_unit.add_time, work_unit.done_time) >
               VPLTIME_FROM_SEC(1)) {
                int i;
                VPLTRACE_LOG_INFO(TRACE_APP, 0,
                                  "Task %d (%p) took "FMT_VPLTime_t"us to complete, "FMT_VPLTime_t" to run.",
                                  work_unit.task_id, work_unit.task,
                                  VPLTime_DiffAbs(work_unit.add_time, work_unit.done_time),
                                  VPLTime_DiffAbs(work_unit.run_time, work_unit.done_time));
                VPLTRACE_LOG_INFO(TRACE_APP, 0,
                                  "Previous %d Tasks in reverse order:",
                                  work_unit.tasks_ahead + 1);
                for(i = 0; i <= work_unit.tasks_ahead; i++) {
                    int old_task_id = (vsTest_q_cap + work_unit.task_id - i - 1) % vsTest_q_cap;
                    vsTest_worker_task* old_task = &(vsTest_task_q[old_task_id]);
                    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                                      "Task %d: %p.",
                                      old_task->task_id, old_task->task);
                }
            }
#endif
        }else if(vsTest_q_size < 0) { // Negative queue size
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "Should never happen: %d", vsTest_q_size);
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex));

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Worker exiting.");
    return NULL;
}

int vsTest_worker_init(int numWorkers)
{
    int rv = 0;

    vsTest_worker_threads = (VPLThread_t*)calloc(sizeof(VPLThread_t), numWorkers);
    num_threads = numWorkers;

    if(vsTest_worker_threads == NULL) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Failed to allocate space for %d workers.", numWorkers);
        goto fail_thread_alloc;
    }

    // initialize queue with initial size
    vsTest_task_q = (vsTest_worker_task*)malloc(VSTEST_Q_INITIAL_CAP *
                                                sizeof(vsTest_worker_task));
    if(vsTest_task_q == NULL) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Failed to allocate task queue.");
        goto fail_alloc;
    }
    vsTest_q_head = 0; vsTest_q_tail = 0; vsTest_q_max = 0;
    vsTest_q_cap = VSTEST_Q_INITIAL_CAP;
    vsTest_q_size = 0;

    VPLCond_Init(&vsTest_worker_condvar);

    goto done;
 fail_alloc:
    free(vsTest_worker_threads);
 fail_thread_alloc:
 done:

    return rv;
}

int vsTest_worker_start(void)
{
    int rv = 0;
    VPLThread_attr_t thread_attr;

    rv = VPLThread_AttrInit(&thread_attr);
    if(rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Failed to initialize thread attributes.");
        goto fail_attr_init;
    }

    VPLThread_AttrSetStackSize(&thread_attr, WORKER_STACK_SIZE);
    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                      "Starting worker threads with stack size %d.",
                      WORKER_STACK_SIZE);

    // spawn worker thread
    vsTest_worker_run = 1;
    for(int i = 0; i < num_threads; i++) {
        rv = VPLThread_Create(&vsTest_worker_threads[i], vsTest_worker, NULL, &thread_attr, "vsTest_worker");
        if(rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "Failed to init worker thread[%d]: %s(%d).",
                             i, strerror(rv), rv);
            goto fail_thread;
        }
        num_threads_running++;
    }

    goto done;
 fail_thread:
    vsTest_worker_stop();
 done:
    VPLThread_AttrDestroy(&thread_attr);
 fail_attr_init:
    return rv;
}

void vsTest_worker_stop(void)
{
    int i;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex));
    if(vsTest_worker_run) {
        vsTest_worker_run = 0;
        VPLCond_Broadcast(&vsTest_worker_condvar);
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex));

    for(i = 0; i < num_threads_running; i++) {
        VPLThread_Join(&vsTest_worker_threads[i], NULL);
    }
}

void vsTest_worker_cleanup(void)
{
    // halt and clean-up the thread.
    vsTest_worker_stop();

    // Delete the tasks.
    // Note: params not freed since structure cannot be known.
    // This leaks memory.
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex));
    free(vsTest_task_q);
    vsTest_task_q = NULL;
    vsTest_q_cap = 0;
    vsTest_q_size = 0;
    free(vsTest_worker_threads);
    vsTest_worker_threads = NULL;
    num_threads = 0;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex));
}

int vsTest_worker_add_task(void (*task)(void*), void* params)
{
    int rv = 0;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex));

    // If the queue is full, reallocate the queue.
    // This shouldn't happen if the queue is sized well initially.
    // It's for insurance.
    if(vsTest_q_cap == vsTest_q_size) {
        vsTest_worker_task* new_q;

        VPLTRACE_LOG_WARN(TRACE_APP, 0,
                          "Reallocating task queue. Exceeded %d tasks queued.",
                          vsTest_q_cap);
        new_q = (vsTest_worker_task*)realloc(vsTest_task_q,
                                             (vsTest_q_cap + VSTEST_Q_GROW_QTY) * sizeof(vsTest_worker_task));
        if(new_q == NULL) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "Failed to reallocate queue to %d tasks.",
                             (vsTest_q_cap + VSTEST_Q_GROW_QTY));
        }
        else {
            // Unfold the queue into the new space.
            // From head...end shifts up to the end of the new space.
            int i;
            vsTest_task_q = new_q;

            for(i = vsTest_q_cap - 1; i >= vsTest_q_head; i--) {
#if DEBUG_WORK_QUEUE
                // Renumber the moving tasks.
                vsTest_task_q[i].task_id += VSTEST_Q_GROW_QTY;
#endif
                memcpy(&(vsTest_task_q[i + VSTEST_Q_GROW_QTY]),
                       &(vsTest_task_q[i]),
                       sizeof(vsTest_worker_task));
            }
            vsTest_q_head += VSTEST_Q_GROW_QTY;
            vsTest_q_cap += VSTEST_Q_GROW_QTY;
            vsTest_q_head %= vsTest_q_cap;
        }
    }

    // Add if there is space in the queue, then signal worker.
    if(vsTest_q_cap > vsTest_q_size) {
        vsTest_task_q[vsTest_q_tail].task = task;
        vsTest_task_q[vsTest_q_tail].params = params;

#if DEBUG_WORK_QUEUE
        vsTest_task_q[vsTest_q_tail].task_id = vsTest_q_tail;
        vsTest_task_q[vsTest_q_tail].tasks_ahead = vsTest_q_size;
        vsTest_task_q[vsTest_q_tail].add_time = VPLTime_GetTimeStamp();
#endif

        vsTest_q_tail = (vsTest_q_tail + 1) % vsTest_q_cap;
        vsTest_q_size++;
        if(vsTest_q_size > vsTest_q_max) {
            vsTest_q_max = vsTest_q_size;
        }
        VPLCond_Signal(&vsTest_worker_condvar);
        VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                            "Queued task. Function %p, context %p.",
                            task, params);
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Failed to queue task: queue at capacity.");
        rv = -1;
    }

    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&vsTest_worker_mutex));

    return rv;
}
