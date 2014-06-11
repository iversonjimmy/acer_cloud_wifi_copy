//
//  Copyright (C) 2007-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplTest.h"
#include "vplTest_suite_common.h"
#include "vpl_fs.h"
#include "vplu_debug.h"
#include "vpl_th.h"

#include <signal.h>
#include <assert.h>
#include <errno.h>

static VPLSem_t interruptSem;
static void
sigintHandler(int sig)
{
    VPLSem_Post(&interruptSem);
}

static void debugCallback(const VPL_DebugMsg_t* data)
{
    VPLTEST_LOG_ERR("debugCallback:%s:%d: %s", data->file, data->line, data->msg);
}

static void monitorCb(VPLFS_MonitorHandle handle,
                      void* eventBuffer,
                      int eventBufferSize,
                      int error)
{
    if(error == VPLFS_MonitorCB_OVERFLOW) {
        VPLTEST_LOG_ERR("%s:%d: Overflow occurred.", __FILE__, __LINE__);
    }else if(error == VPLFS_MonitorCB_OK) {
        int bufferSize = 0;
        void* currEventBuf = eventBuffer;
        while(bufferSize < eventBufferSize) {
            VPLFS_MonitorEvent* monitorEvent = (VPLFS_MonitorEvent*) currEventBuf;
            switch(monitorEvent->action) {
            case VPLFS_MonitorEvent_FILE_ADDED:
                VPLTEST_LOG("File added:%s", monitorEvent->filename);
                break;
            case VPLFS_MonitorEvent_FILE_REMOVED:
                VPLTEST_LOG("File removed:%s", monitorEvent->filename);
                break;
            case VPLFS_MonitorEvent_FILE_MODIFIED:
                VPLTEST_LOG("File modified:%s", monitorEvent->filename);
                break;
            case VPLFS_MonitorEvent_FILE_RENAMED:
                VPLTEST_LOG("File renamed:%s->%s", monitorEvent->filename, monitorEvent->moveTo);
                break;
            case VPLFS_MonitorEvent_NONE:
            default:
                VPLTEST_LOG("action not supported:%d, %s", monitorEvent->action, monitorEvent->filename);
            }

            if(monitorEvent->nextEntryOffsetBytes == 0) {
                VPLTEST_LOG("%s:%d: Done traversing events.\n", __FILE__, __LINE__);
                break;
            }
            VPLTEST_LOG("Next event %d", monitorEvent->nextEntryOffsetBytes);
            currEventBuf = (u8*)currEventBuf + monitorEvent->nextEntryOffsetBytes;
            bufferSize += monitorEvent->nextEntryOffsetBytes;
        }
    }else{
        VPLTEST_LOG_ERR("%s:%d: Unhandled error:%d.", __FILE__, __LINE__, error);
    }
}

#ifdef WIN32
static const char* TEST_DIR = "C:/testMonitorDir";
#else
static const char* TEST_DIR = "/tmp/testMonitorDir";
#endif

int main(int argc, char* argv[])
{
    int rc;
    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(TEST_DIR, &statBuf);
    if(rc != 0 || statBuf.type != VPLFS_TYPE_DIR) {
        VPLTEST_LOG_ERR("Require directory:%s, exiting", TEST_DIR);
        return -7;
    }
    VPLFS_MonitorHandle monitorHandle_out;

    VPL_RegisterDebugCallback(debugCallback);

    VPLTEST_LOG("Build ID = %s", VPL_GetBuildId());

    rc = VPLSem_Init(&interruptSem, 1, 0);
    if (rc < 0) {
        printf("Error: sem_init:%d (%d, %s)\n", rc, errno, strerror(errno));
        return -1;
    }

    VPLTEST_LOG("Monitoring the directory:%s", TEST_DIR);
    VPLTEST_LOG("VPLFS_MonitorInit: 1st time");
    rc = VPLFS_MonitorInit();
    if(rc != 0) {
        VPLTEST_LOG_ERR("FAIL: VPLFS_MonitorInit:%d", rc);
    }
    VPLTEST_LOG("VPLFS_MonitorInit: 2nd time");
    rc = VPLFS_MonitorInit();
    if(rc != 0) {
        VPLTEST_LOG_ERR("FAIL: VPLFS_MonitorInit:%d", rc);
    }
    VPLTEST_LOG("VPLFS_MonitorInit: 3rd time");
    rc = VPLFS_MonitorInit();
    if(rc != 0) {
        VPLTEST_LOG_ERR("FAIL: VPLFS_MonitorInit:%d", rc);
    }

    rc = VPLFS_MonitorDir(TEST_DIR,
                          20,
                          monitorCb,
                          &monitorHandle_out);
    if(rc != 0) {
        VPLTEST_LOG_ERR("FAIL: VPLFS_MonitorDir:%d", rc);
    }

    (void)signal(SIGINT, sigintHandler);
    while (VPLSem_TryWait(&interruptSem)) {
        VPLThread_Sleep(VPLTime_FromSec(2));
        static int seconds = 0;
        printf("Monitoring %s.  %d sec elapsed. Send SIGINT to quit.  (Control-c)\n",
               TEST_DIR, seconds);
        seconds += 2;
    }
    printf("Received SIGINT.  Exiting.\n");

    rc = VPLFS_MonitorDirStop(monitorHandle_out);
    if(rc != 0) {
        VPLTEST_LOG_ERR("FAIL: VPLFS_MonitorDirStop:%d", rc);
    }
    VPLTEST_LOG("VPLFS_MonitorDestroy: 1st time");
    rc = VPLFS_MonitorDestroy();
    if(rc != 0) {
        VPLTEST_LOG_ERR("FAIL: VPLFS_MonitorDestroy:%d", rc);
    }
    VPLTEST_LOG("VPLFS_MonitorDestroy: 2nd time");
    rc = VPLFS_MonitorDestroy();
    if(rc != 0) {
        VPLTEST_LOG_ERR("FAIL: VPLFS_MonitorDestroy:%d", rc);
    }
    VPLTEST_LOG("VPLFS_MonitorDestroy: 3rd time");
    rc = VPLFS_MonitorDestroy();
    if(rc != 0) {
        VPLTEST_LOG_ERR("FAIL: VPLFS_MonitorDestroy:%d", rc);
    }

    //    if (vplTest_getTotalErrCount() > 0) {
    //        printf("*** TEST SUITE FAILED, %d error(s)\n", vplTest_getTotalErrCount());
    //    }
    VPLSem_Destroy(&interruptSem);
    VPLTEST_LOG("\nEXIT\n");
    return 0;  //vplTest_getTotalErrCount();
}
