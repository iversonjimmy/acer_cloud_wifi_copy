//
//  Copyright (C) 2007-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_TEST_H__
#define __VPLEX_TEST_H__

#include "vplex_plat.h"
#include "vplTest_common.h"

#ifdef __cplusplus
extern "C" {
#endif

const char* vplexErrToString(int errCode);

#define FMT_VPLEX_ERR  "%d (%s)"
#define ARG_VPLEX_ERR(err)  (err), vplexErrToString(err)

#undef ARG_VPL_ERR
#define ARG_VPL_ERR  ARG_VPLEX_ERR

// List of tests
#if defined(VPL_PLAT_IS_WINRT) || defined(ANDROID)
int testVPLex(int argc, char* argv[]);
#endif
void testVPLMath(void);
void testVPLMemHeap(void);
void testVPLSafeSerialization(void);
void testVPLSerialization(void);
void testVPLXmlReader(void);
void testVPLXmlReaderUnescape(void);
void testVPLXmlWriter(void);
void testVPLHttp(void);
#if !defined(_WIN32) && !defined(ANDROID)
void testVPLWstring(void);
#endif
#ifdef __cplusplus
}
#endif

#endif // include guard
