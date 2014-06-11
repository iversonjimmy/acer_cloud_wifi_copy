/*
*
*                Copyright (C) 2005,2008 BroadOn Communications Corp.
*
*   These coded instructions, statements, and computer programs contain
*   unpublished  proprietary information of BroadOn Communications Corp.,
*   and  are protected by Federal copyright law. They may not be disclosed
*   to  third  parties or copied or duplicated in any form, in whole or in
*   part, without the prior written consent of BroadOn Communications Corp.
*
*/

#include "vplex_plat.h"

#if defined(VPLTRACE_NO_TRACE) // VPLTRACE_NO_TRACE: strip VPL trace support.

#include "vpl_types.h"
#include "vpl_th.h"
#include "vplex_trace.h"

// Provide do-nothing dummy stubs, so that applications compiled with tracing
// enabled (by not defining VPLTRACE_NO_TRACE) can link against this library even if
// it is built with VPLTRACE_NO_TRACE.

// TODO: keep in synch with vplex_trace.h.
// TODO: rework vplex_trace.h with a hook to provide prototypes even
// when VPLTRACE_NO_TRACE is defined.

//==============================================
// Replace the empty #defines with prototypes.

#undef VPLTrace_Init
void VPLTrace_Init(VPLTrace_LogCallback_t logCallback);

#undef VPLTrace_SetEnable
bool VPLTrace_SetEnable(bool enable);

#undef VPLTrace_SetBaseLevel
VPLTrace_level VPLTrace_SetBaseLevel(VPLTrace_level level);

#undef VPLTrace_SetShowTimeAbs
bool VPLTrace_SetShowTimeAbs(bool show_time);

#undef VPLTrace_SetShowTimeRel
bool VPLTrace_SetShowTimeRel(bool show_time);

#undef VPLTrace_SetShowThread
bool VPLTrace_SetShowThread(bool show_thread);

#undef VPLTrace_SetGroupEnable
bool VPLTrace_SetGroupEnable(int grp_num, bool enable);

#undef VPLTrace_SetBaseLevel
VPLTrace_level VPLTrace_SetBaseLevel(VPLTrace_level level);

#undef VPLTrace_SetSgLevel
VPLTrace_level VPLTrace_SetSgLevel(int grp, int sub_grp, VPLTrace_level level);

#undef VPLTrace_SetGroupEnableMask
unsigned VPLTrace_SetGroupEnableMask(int grp, unsigned mask);

#undef VPLTrace_SetTraceFile
const char* VPLTrace_SetTraceFile(const char* filename);

#undef VPLTrace_SetGroupDisableMask
unsigned VPLTrace_SetGroupDisableMask(int grp, unsigned mask);

#undef VPLTrace_GetSgLevel
VPLTrace_level VPLTrace_GetSgLevel(int grp_num, int sub_grp);

#undef VPLTrace_GetGroupLevel
VPLTrace_level VPLTrace_GetGroupLevel(int grp_num);

#undef VPLTrace_On
bool VPLTrace_On(VPLTrace_level level, int grp, int sub_grp);

#undef VPLTrace_LevelStr
const char* VPLTrace_LevelStr(VPLTrace_level level);


#undef VPLTrace_TraceEx
void VPLTrace_TraceEx(VPLTrace_level level,
                      const char* sourceFilename,
                      int lineNum,
                      const char* functionName,
                      bool appendNewline,
                      int grp_num,
                      int sub_grp,
                      const char* fmt,
                      ...) ATTRIBUTE_PRINTF(8, 9);

//===================================

void VPLTrace_Init(VPLTrace_LogCallback_t logCallback)  { return ; }
bool VPLTrace_SetEnable(bool enable)                    { return true; }
VPLTrace_level VPLTrace_SetBaseLevel(VPLTrace_level level)  { return (VPLTrace_level)0; }
bool VPLTrace_SetShowTimeAbs(bool show_time)            { return false; }
bool VPLTrace_SetShowTimeRel(bool show_time)            { return false; }
bool VPLTrace_SetShowThread(bool show_thread)           { return false; }

const char* VPLTrace_SetTraceFile(const char* filename) { return ""; }

bool VPLTrace_SetGroupEnable(int grp_num, bool enable)  { return false; }
VPLTrace_level VPLTrace_SetGroupLevel(int grp, VPLTrace_level level)
{
    return (VPLTrace_level)0;
}

VPLTrace_level VPLTrace_SetSgLevel(int grp, int sub_grp, VPLTrace_level level)
{
    return (VPLTrace_level)0;
}

unsigned VPLTrace_SetGroupEnableMask(int grp, unsigned mask) {return 0; }

unsigned VPLTrace_SetGroupDisableMask(int grp, unsigned mask) { return 0; }

VPLTrace_level VPLTrace_GetSgLevel(int grp_num, int sub_grp) { return false; }

VPLTrace_level VPLTrace_GetGroupLevel(int grp_num) { return (VPLTrace_level)0; }

bool VPLTrace_On(VPLTrace_level level, int grp, int sub_grp) { return false; }

const char* VPLTrace_LevelStr(VPLTrace_level level){ UNUSED(level); return "? "; }

void VPLTrace_TraceEx(VPLTrace_level level,
                      const char* sourceFilename,
                      int lineNum,
                      const char* functionName,
                      bool appendNewline,
                      int grp_num,
                      int sub_grp,
                      const char* fmt,
                      ...) ATTRIBUTE_PRINTF(8, 9);
{
    return;
}


void VPLTrace_DumpBuf(VPLTrace_level level,
                      const char* sourceFilename,
                      int lineNum,
                      const char* functionName,
                      bool appendNewline,
                      int grp_num,
                      int sub_grp,
                      const char* buf,
                      size_t len);
{
    return;
}

#endif // defined(VPLTRACE_NO_TRACE) // VPLTRACE_NO_TRACE: strip VPL trace support.
