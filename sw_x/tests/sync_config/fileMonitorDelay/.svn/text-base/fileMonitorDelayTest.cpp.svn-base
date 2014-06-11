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
#include "FileMonitorDelayQ.hpp"
#include "vpl_fs.h"
#include "vplu_debug.h"
#include "vpl_th.h"

#include <signal.h>
#include <assert.h>
#include <errno.h>

#include <log.h>

static VPLSem_t interruptSem;
static void
sigintHandler(int sig)
{
    VPLSem_Post(&interruptSem);
}

static void debugCallback(const VPL_DebugMsg_t* data)
{
    LOG_ERROR("debugCallback:%s:%d: %s", data->file, data->line, data->msg);
}

static void fMonitorDelayCb(VPLFS_MonitorHandle handle,
                            void* eventBuffer,
                            int eventBufferSize,
                            int error,
                            void* ctx)
{
    printf("ctx:%s\n", (char*)ctx);
    if(error == VPLFS_MonitorCB_OVERFLOW) {
        LOG_ERROR("Overflow occurred.");
    }else if(error == VPLFS_MonitorCB_OK) {
        int bufferSize = 0;
        void* currEventBuf = eventBuffer;
        while(bufferSize < eventBufferSize) {
            VPLFS_MonitorEvent* monitorEvent = (VPLFS_MonitorEvent*) currEventBuf;
            switch(monitorEvent->action) {
            case VPLFS_MonitorEvent_FILE_ADDED:
                LOG_INFO("File added:%s", monitorEvent->filename);
                break;
            case VPLFS_MonitorEvent_FILE_REMOVED:
                LOG_INFO("File removed:%s", monitorEvent->filename);
                break;
            case VPLFS_MonitorEvent_FILE_MODIFIED:
                LOG_INFO("File modified:%s", monitorEvent->filename);
                break;
            case VPLFS_MonitorEvent_FILE_RENAMED:
                LOG_INFO("File renamed:%s->%s", monitorEvent->filename, monitorEvent->moveTo);
                break;
            case VPLFS_MonitorEvent_NONE:
            default:
                LOG_INFO("action not supported:%d, %s", monitorEvent->action, monitorEvent->filename);
            }
            if(monitorEvent->nextEntryOffsetBytes == 0) {
                LOG_INFO("Done traversing events.");
                break;
            }

            LOG_INFO("Next event %d", monitorEvent->nextEntryOffsetBytes);
            currEventBuf = (u8*)currEventBuf + monitorEvent->nextEntryOffsetBytes;
            bufferSize +=   monitorEvent->nextEntryOffsetBytes;
        }
    }else{
        LOG_ERROR("Unhandled error:%d.", error);
    }
}

#ifdef WIN32
static const char* TEST_DIR = "C:/testMonitorDir";
static const char* TEST_DIR_2 = "C:/testMonitorDir_2";
#else
static const char* TEST_DIR = "/tmp/testMonitorDir";
static const char* TEST_DIR_2 = "/tmp/testMonitorDir_2";
#endif

int main(int argc, char* argv[])
{
    int rc;
    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(TEST_DIR, &statBuf);
    if(rc != 0 || statBuf.type != VPLFS_TYPE_DIR) {
        LOG_ERROR("Require directory:%s, exiting", TEST_DIR);
        return -7;
    }
    rc = VPLFS_Stat(TEST_DIR_2, &statBuf);
    if(rc != 0 || statBuf.type != VPLFS_TYPE_DIR) {
        LOG_ERROR("Require directory:%s, exiting", TEST_DIR_2);
        return -7;
    }
    u32 delayAmountSec = 5;
    FileMonitorDelayQ fMonitorDelayQ(VPLTime_FromSec(delayAmountSec));
    VPLFS_MonitorHandle monitorHandle_out;
    VPLFS_MonitorHandle monitorHandle_out_2;

    VPL_RegisterDebugCallback(debugCallback);

    LOG_INFO("Build ID = %s", VPL_GetBuildId());

    rc = VPLSem_Init(&interruptSem, 1, 0);
    if (rc < 0) {
        LOG_ERROR("Error: sem_init:%d (%d, %s)", rc, errno, strerror(errno));
        return -1;
    }

    LOG_INFO("================== Monitoring the directory:%s ================", TEST_DIR);
    LOG_INFO("================== Monitoring the directory:%s ================", TEST_DIR_2);
    LOG_INFO("======================= DELAY AMOUNT:%d =======================", delayAmountSec);
    rc = fMonitorDelayQ.Init();
    if(rc != 0) {
        LOG_ERROR("FAIL: fMonitorDelayQ.Init():%d", rc);
    }

    rc = fMonitorDelayQ.AddMonitor(TEST_DIR,
                                   20,
                                   fMonitorDelayCb,
                                   (void*)"DIR CTX 1",
                                   &monitorHandle_out);
    if(rc != 0) {
        LOG_ERROR("FAIL: fMonitorDelayQ.AddMonitor:%d", rc);
    }

    rc = fMonitorDelayQ.AddMonitor(TEST_DIR_2,
                                   20,
                                   fMonitorDelayCb,
                                   (void*)"DIR CTX 2",
                                   &monitorHandle_out_2);
    if(rc != 0) {
        LOG_ERROR("FAIL: fMonitorDelayQ.AddMonitor:%d", rc);
    }


    (void)signal(SIGINT, sigintHandler);
    while (VPLSem_TryWait(&interruptSem)) {
        VPLThread_Sleep(VPLTime_FromSec(2));
        static int seconds = 0;
        printf("Monitoring. %d sec elapsed. Send SIGINT to quit.  (Control-c)\n",
               seconds);
        seconds += 2;
    }
    LOG_INFO("Received SIGINT.  Exiting.");

    rc = fMonitorDelayQ.RemoveMonitor(monitorHandle_out);
    if(rc != 0) {
        LOG_ERROR("FAIL: fMonitorDelayQ.RemoveMonitor:%d", rc);
    }
    rc = fMonitorDelayQ.RemoveMonitor(monitorHandle_out_2);
    if(rc != 0) {
        LOG_ERROR("FAIL: fMonitorDelayQ.RemoveMonitor:%d", rc);
    }
    rc = fMonitorDelayQ.Shutdown();
    if(rc != 0) {
        LOG_ERROR("FAIL: fMonitorDelayQ.Shutdown():%d", rc);
    }

    //    if (vplTest_getTotalErrCount() > 0) {
    //        printf("*** TEST SUITE FAILED, %d error(s)\n", vplTest_getTotalErrCount());
    //    }
    VPLSem_Destroy(&interruptSem);
    LOG_INFO("\nEXIT\n");
    return 0;  //vplTest_getTotalErrCount();
}

