//
//  Copyright (C) 2007-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL_TEST_H__
#define __VPL_TEST_H__

#include "vplTest_common.h"
#include "vpl_plat.h"
#include "vpl_time.h"

/// Instead of requiring #testVPLTime() to always run first, we can grab
/// the initial value from #VPLTime_GetTimeStamp() early on.
extern VPLTime_t initialTimeStamp;

// List of tests
#ifdef VPL_PLAT_IS_WINRT
int testVPL(int argc, char* argv[]);
#endif
void testVPLPlatformCompliance(void);
void testVPLThread(void);
void testVPLMutex(void);
void testVPLSem(void);
void testVPLCond(void);
void testVPLTime(void);
void testVPLSocket(void);
void testVPLWstring(void);
void testVPLConv(void);
void testVPLNetwork(void);
void testVPLFS(void);
void testVPLSlimRWLock(void);
void testVPLDL(void);
void testVPLPlat(void);
void testVPLShm(void);

#endif // include guard
