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

#ifndef __VSTEST_WORKER_H__
#define __VSTEST_WORKER_H__

#ifdef __cplusplus
extern "C" {
#endif

/// Initialize state for the task queue and worker pool
int vsTest_worker_init(int numWorkers);

/// Start the asynchronous worker threads. Pending tasks begin processing.
int vsTest_worker_start(void);

/// Stop the asynchronous worker threads. State preserved.
void vsTest_worker_stop(void);

/// Cleanup residual worker thread state. This will stop the worker if not
/// already stopped.
/// This operation will leak memory. Do only before ending process.
void vsTest_worker_cleanup(void);

/// Add a task to the worker thread's task queue
/// On completion of the task, the task function must free any params.
int vsTest_worker_add_task(void (*task)(void*), void* params);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // include guard
