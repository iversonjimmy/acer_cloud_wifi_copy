//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vplu_debug.h"
#include "vpl_string.h"

static VPL_DebugCallback_t _callback = NULL;

void VPL_RegisterDebugCallback(VPL_DebugCallback_t callback)
{
    // TODO: Assumes atomic assignment of pointers.
    _callback = callback;
}

void VPL_ReportMsg(VPL_DebugLevel_t level, const char* file, int line, const char* msgFmt, ...)
{
    // Copy callback pointer to the stack to reduce race conditions.
    // TODO: Assumes atomic assignment of pointers.
    VPL_DebugCallback_t myCallback = _callback;
    if (myCallback != NULL) {
        va_list args;
        char* actualMsg;
        int actualMsgLen;
        VPL_DebugMsg_t data;
        
        // Figure out how much space we need for the actual message.
        va_start(args, msgFmt);
        actualMsgLen = VPL_vsnprintf(NULL, 0, msgFmt, args);
        va_end(args);
        
        // Allocate the actual message buffer.
        actualMsg = (char*)malloc(actualMsgLen + 1);
        if (actualMsg == NULL) {
            data.msg = "Alloc failed while reporting!";
        } else {
            // Fill in the actual message.
            va_start(args, msgFmt);
            VPL_vsnprintf(actualMsg, actualMsgLen + 1, msgFmt, args);
            va_end(args);
            data.msg = actualMsg;
        }
        // Fill in the rest of the data.
        data.threadId = VPLThread_Self();
        data.file = file;
        data.line = line;
        data.level = level;
        
        // Invoke the callback.
        myCallback(&data);
        
        // Free the actual message buffer.
        free(actualMsg);
    }
}
