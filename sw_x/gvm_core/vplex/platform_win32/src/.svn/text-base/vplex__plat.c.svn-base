/*
*                Copyright (C) 2005, BroadOn Communications Corp.
*
*   These coded instructions, statements, and computer programs contain
*   unpublished  proprietary information of BroadOn Communications Corp.,
*   and  are protected by Federal copyright law. They may not be disclosed
*   to  third  parties or copied or duplicated in any form, in whole or in
*   part, without the prior written consent of BroadOn Communications Corp.
*
*/

#include "vplex_plat.h"
#include "vplex_assert.h"
#include "vplex_trace.h"
#include "vpl_th.h"
#include <assert.h>

#ifndef _WIN32
#   error "_WIN32 should be defined"
#endif

#ifdef VPL_NO_ASSERT // ASSERT() is to be a no-op
// do nothing

#else // !VPL_NO_ASSERT -- ASSERT() is to be a no-op
void VPLAssert_Failed(char const* file, char const* func, int line, char const* formatMsg, ...)
{
    va_list args;
    
    /// Prevent interleaving by other threads.
#ifdef USE_STDOUT_LOCKING
    _lock_file(stdout);
#endif
    //! *** When exiting scope >>>
    //! Always: unlock stdout

    if (func != NULL) {
        printf("ASSERTION FAILED: %s:%s:%d:\n", file, func, line);
    }
    else {
        printf("ASSERTION FAILED: %s:%d:\n", file, line);
    }

    va_start(args, formatMsg);
    vprintf(formatMsg, args);
    va_end(args);

    printf("\n");

    // Also log to VPLTrace, in case stdout is being discarded.

    VPLTrace_TraceEx(TRACE_ALWAYS, file, line, func, true, VPLTRACE_GRP_VPL, VPL_SG_MISC,
            "ASSERTION FAILED!");
    // Technically, we should really use va_copy to go through the args twice, but this should
    // always be safe for Windows.
    va_start(args, formatMsg);
    VPLTrace_VTraceEx(TRACE_ALWAYS, file, line, func, true, VPLTRACE_GRP_VPL, VPL_SG_MISC,
            formatMsg, args);
    va_end(args);

    //! >>> unlock ***
#ifdef USE_STDOUT_LOCKING
    _unlock_file(stdout);
#endif


#ifndef VPL_NO_BREAK_ON_ASSERT
    #if defined(_DEBUG) && defined(_MSC_VER) && !defined(_WIN64)
        // This instruction seems to be ignored in release builds anyway.
        //bug 4989: According to the MSDN page http://msdn.microsoft.com/en-us/library/4ks26t93(v=vs.110).aspx,
        //the inline assembler only support x86 processors.
        __debugbreak();
    #else
        assert(0);
    #endif
#endif
}
#endif  // !VPL_NO_ASSERT -- ASSERT() is to be a no-op

//=============================================================



void VPLStack_Trace(void)
{
    // not currently supported on this platform
}

