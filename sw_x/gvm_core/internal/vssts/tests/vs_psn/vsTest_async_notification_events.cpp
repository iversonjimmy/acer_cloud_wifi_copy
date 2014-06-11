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

#include <ctype.h>
#include <string>

#include "vpl_time.h"
#include "vpl_th.h"
#include "vplex_assert.h"
#include "vplex_trace.h"
#include "vplex_user.h"

#include "vssts.hpp"
#include "vssts_error.hpp"
//#include "vss_comm.h"

//#include "vsTest_vscs_common.hpp"
#include "vsTest_async_notification_events.hpp"

using namespace std;

extern const char* vsTest_curTestName;
const static int EVENT_TIMEOUT = 35;//sec
const static int CLIENT_INACTIVE_TIMEOUT_SEC = 300;
const static int OBJECT_INACTIVE_TIMEOUT_SEC1 = 100;
const static int OBJECT_INACTIVE_TIMEOUT_SEC2 = 200;
static VSSI_Object handle;

typedef struct  {
    VPLSem_t sem;
    int rv;
} async_test_context_t;

typedef struct  {
    VPLSem_t sem;
    VSSI_NotifyMask mask;
} async_notification_context_t;

static void async_test_callback(void* ctx, VSSI_Result rv)
{
    async_test_context_t* context = (async_test_context_t*)ctx;

    context->rv = rv;
    VPLSem_Post(&(context->sem));
}

static void async_notification_callback(void* ctx, 
                                        VSSI_NotifyMask mask,
                                        const char* message,
                                        u32 message_length)
{
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Event comes in, type: "FMTu64, mask);
    async_notification_context_t* context = (async_notification_context_t*)ctx;
    // since we don't care VSSI_NOTIFY_DISCONNECTED_EVENT event
    if(mask != VSSI_NOTIFY_DISCONNECTED_EVENT) {
        // Track the object version for debuging
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Object version "FMTu64, VSSI_GetVersion(handle));
        context->mask = mask;
        VPLSem_Post(&(context->sem));
    }
}

static const char vsTest_async_notification_events[] = "Async Notification Events Test";
int test_async_notification_events(u64 user_id,
                                   u64 dataset_id,
                                   bool run_async_client_object_test)
{
    int rv = 0;
    int rc = 0;
    async_test_context_t test_context;
    async_notification_context_t notify_context;
    VSSI_NotifyMask mask = 0;
    VSSI_NotifyMask returned_mask = 0;

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    VPL_SET_UNINITIALIZED(&(notify_context.sem));

    vsTest_curTestName = vsTest_async_notification_events;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to create semaphore.");
        return rv+1;
    }
    
    if(VPLSem_Init(&(notify_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to create semaphore.");
        return rv+1;
    }

    VSSI_Delete_Deprecated(user_id, dataset_id, 
                         &test_context, async_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Delete failed: %d.",
                         rc);
        rv++;
        goto fail_open;
    }

    // We wait here for events being sent out from previous test.
    // This tries to clean event queue so that previous test doesn't impact the result
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Wait event queue to be cleaned for %d secs", EVENT_TIMEOUT);
    VPLThread_Sleep(VPLTime_FromSec(EVENT_TIMEOUT));

    // Open dataset
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "BEFORE OPEN OBJ ");
    VSSI_OpenObjectTS(user_id, dataset_id,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, async_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "Open object failed: %d.",
                    rc);
        rv++;
        goto fail_open;
    }

    mask = VSSI_NOTIFY_DATASET_CHANGE_EVENT;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Set VSSI_NOTIFY_DATASET_CHANGE_EVENT Event, set mask "FMTu64, mask);
    VSSI_SetNotifyEvents(handle, &mask, &notify_context,
                         async_notification_callback,
                         &test_context, async_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Set Notify Events returned result %d.",
                         rc);
        rv++;
        goto fail;
    }

    if(mask != VSSI_NOTIFY_DATASET_CHANGE_EVENT) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Set Notify Events returned mask "FMTu64". Expected "FMTu64,
                         mask,
                         mask);
        rv++;
        goto fail;
    } 
 
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Get VSSI_NOTIFY_DATASET_CHANGE_EVENT Event");
    VSSI_GetNotifyEvents(handle, &returned_mask,
                         &test_context, async_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Get Notify Events returned result %d.",
                         rc);
        rv++;
        goto fail;
    }

    if(mask != returned_mask) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Get Notify Events returned mask "FMTu64". Expected "FMTu64,
                         returned_mask,
                         mask);
        rv++;
        goto fail;
    }

    // Track the object version for debuging
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Object version "FMTu64, VSSI_GetVersion(handle));

    //mkdir
    {
        VSSI_MkDir2(handle, "d", 0,
                    &test_context, async_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
    }
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Write operation failed with result %d",
                         rc);
        rv++;
        goto fail;
    }

    // Track the object version for debuging
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Object version "FMTu64, VSSI_GetVersion(handle));

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Wait Notification events.");
    VPLSem_Wait(&(notify_context.sem));
    if(notify_context.mask != VSSI_NOTIFY_DATASET_CHANGE_EVENT) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Notification events failed with result "FMTu64,
                         notify_context.mask);
        rv++;
        goto fail;   
    }
    
    //test client obj life time
    if(run_async_client_object_test
       // it is meaningful only when CLIENT_INACTIVE_TIMEOUT_SEC < OBJECT_INACTIVE_TIMEOUT_SEC2,
       // or vss_object is timed-out eariler than vss_client, and it returns VSSI_BADOBJ
       && CLIENT_INACTIVE_TIMEOUT_SEC < OBJECT_INACTIVE_TIMEOUT_SEC2) {

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Sleep testing thread for %d seconds to trigger client object timeout.",
                            CLIENT_INACTIVE_TIMEOUT_SEC + EVENT_TIMEOUT);

        sleep(CLIENT_INACTIVE_TIMEOUT_SEC + EVENT_TIMEOUT);
        //mkdir
        {
            VSSI_MkDir2(handle, "d1", 0,
                        &test_context, async_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
        }
        if(rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "Write operation failed with result %d",
                             rc);
            rv++;
            goto fail;
        }

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Wait Notification events.");
        VPLSem_Wait(&(notify_context.sem));
        if(notify_context.mask != VSSI_NOTIFY_DATASET_CHANGE_EVENT) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "Notification events failed with result "FMTu64,
                             notify_context.mask);
            rv++;
            goto fail;   
        }
    }
    //clear the mask, make sure there are no events occur.
    mask = 0;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Clear Events, set mask "FMTu64, mask);
    VSSI_SetNotifyEvents(handle, &mask, &notify_context,
                         async_notification_callback,
                         &test_context, async_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Clear Events returned result %d.",
                         rc);
        rv++;
        goto fail;
    }

    if(mask != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Clear Events returned mask "FMTu64". Expected "FMTu64,
                         mask,
                         mask);
        rv++;
        goto fail;
    } 
 
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Get Clear Event");
    VSSI_GetNotifyEvents(handle, &returned_mask,
                         &test_context, async_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Get Clear Events returned result %d.",
                         rc);
        rv++;
        goto fail;
    }

    if(mask != returned_mask) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Get Clear Events returned mask "FMTu64". Expected "FMTu64,
                         returned_mask,
                         mask);
        rv++;
        goto fail;
    }

    // Track the object version for debuging
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Object version "FMTu64, VSSI_GetVersion(handle));

    //mkdir
    {
        VSSI_MkDir2(handle, "d2", 0,
                    &test_context, async_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
    }
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Write operation failed with result %d",
                         rc);
        rv++;
        goto fail;
    }

    // Track the object version for debuging
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Object version "FMTu64, VSSI_GetVersion(handle));

    //expect no events occur.
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Wait Notification events for %d secs.", EVENT_TIMEOUT);
    rc = VPLSem_TimedWait(&(notify_context.sem),VPLTime_FromSec(EVENT_TIMEOUT));
    if(rc != VPL_ERR_TIMEOUT) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "NO Notification events failed with result %d.",
                         rc);
        rv++;
        goto fail;   
    }

    VSSI_CloseObject(handle, &test_context, async_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Close object failed: %d.",
                         rc);
        rv++;
        goto fail;
    }

    //vss object life time test
    if(run_async_client_object_test) {

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Sleep testing thread for %d seconds to trigger vss object timeout.",
                            OBJECT_INACTIVE_TIMEOUT_SEC1);
        VSSI_File fileHandle = NULL;
        VSSI_OpenObjectTS(user_id, dataset_id, 
                             VSSI_READWRITE | VSSI_FORCE, &handle,
                             &test_context, async_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                        "Open object failed: %d.",
                        rc);
            rv++;
            goto fail_open;
        }

        //do open file
        {
            VSSI_OpenFile(handle, "a",
                          VSSI_FILE_OPEN_CREATE | VSSI_FILE_OPEN_READ | VSSI_FILE_OPEN_WRITE, 0, &fileHandle,
                          &test_context, async_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
        }
        if(rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "OpenFile operation failed with result %d",
                             rc);
            rv++;
            goto fail;
        }

        sleep(OBJECT_INACTIVE_TIMEOUT_SEC1);
    
        VSSI_Object handle2; 
        VSSI_OpenObjectTS(user_id, dataset_id,
                             VSSI_READWRITE | VSSI_FORCE, &handle2,
                             &test_context, async_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                        "Open object failed: %d.",
                        rc);
            rv++;
            goto fail_open;
        }
    
        //do open file, wait not enough, should be denied.
        {
            VSSI_OpenFile(handle2, "a",
                          VSSI_FILE_OPEN_READ, 0, &fileHandle,
                          &test_context, async_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
        }
        if(rc == 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "OpenFile operation failed with result %d expect failed.",
                             rc);
            rv++;
            goto fail;
        }

        VSSI_CloseObject(handle2, &test_context, async_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Close object failed: %d.",
                             rc);
            rv++;
            goto fail;
        }

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Sleep testing thread for %d seconds to trigger vss object timeout.",
                            OBJECT_INACTIVE_TIMEOUT_SEC2);
        sleep(OBJECT_INACTIVE_TIMEOUT_SEC2);
        //close object, vss object is timed-out, shoudl fail
        VSSI_CloseObject(handle, &test_context, async_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc == VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Close object failed: %d expect failed.",
                             rc);
            rv++;
            goto fail;
        }

        // Open dataset again
        VSSI_OpenObjectTS(user_id, dataset_id,
                             VSSI_READWRITE | VSSI_FORCE, &handle2,
                             &test_context, async_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                        "Open object failed: %d.",
                        rc);
            rv++;
            goto fail_open;
        }


            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                             "OpenFile operation before");
        //do open file, vss object is timed-out, should be able to access
        {
            VSSI_OpenFile(handle2, "a",
                          VSSI_FILE_OPEN_READ, 0, &fileHandle,
                          &test_context, async_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
        }
        if(rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "OpenFile operation failed with result %d",
                             rc);
            rv++;
            goto fail;
        }

            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                             "OpenFile operation after");
        VSSI_CloseObject(handle2, &test_context, async_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Close object failed: %d.",
                             rc);
            rv++;
            goto fail;
        }
    }


    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Finished test: %s.",
                        vsTest_curTestName);
 fail_open:
 fail:
    VPLSem_Destroy(&(test_context.sem));
    VPLSem_Destroy(&(notify_context.sem));
    return rv;
}

