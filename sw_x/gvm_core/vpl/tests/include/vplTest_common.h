//
//  Copyright (C) 2007-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL_TEST_COMMON_H__
#define __VPL_TEST_COMMON_H__

//============================================================================
/// @file
/// Functionality shared by all VPL-based tests and test suites.
//============================================================================

#include <vplu.h>
#include <vplu_types.h>

#include <stdlib.h>
#include <setjmp.h>

#ifdef VPL_PLAT_IS_WINRT
// Support writing log files to app's LocalFolder on WinRT
// Currently use fixed size buffer to write log file
// TODO: allocate accurate size buffer for var args when writing logs
#include "vplex_file.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------

const char* vplErrToString(int errCode);
#define FMT_VPL_ERR  "%d (%s)"
#define ARG_VPL_ERR(err)  (err), vplErrToString(err)

//----------------------------------------------------------------------------

#ifdef VPL_PLAT_IS_WINRT

void VPLTEST_LOGINIT(const char* logPath);
void VPLTEST_LOGCLOSE();
void VPLTEST_LOGWRITE(const char* fmt, ...);

#elif defined(ANDROID)
#  include <android/log.h>
#endif
//----------------------------------------------------------------------------

/// @deprecated #VPLTEST_LOG automatically adds newlines.
#ifdef VPL_PLAT_IS_WINRT
#  define VPLTEST_PRINT( fmt, ...)  VPLTEST_LOGWRITE( "### %s:%d:"fmt, __FILE__, __LINE__, ##__VA_ARGS__);

#  define VPLTEST_LOG( fmt, ...)  \
    VPLTEST_LOGWRITE( "### %s:%d:"fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__);

#  define VPLTEST_DEBUG( fmt, ...)  \
    VPLTEST_LOGWRITE( "--- %s:%d:"fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__);

#elif defined(ANDROID)
#  define  VPLTEST_PRINT(fmt, ...) __android_log_print ( ANDROID_LOG_DEBUG, "VPLTEST", \
        "### %s:%d:" fmt,  __FILE__, __LINE__, ##__VA_ARGS__)

#  define  VPLTEST_LOG(fmt, ...) __android_log_print ( ANDROID_LOG_DEBUG, "VPLTEST", \
        "### %s:%d:" fmt,  __FILE__, __LINE__, ##__VA_ARGS__)

#  define  VPLTEST_DEBUG(fmt, ...) __android_log_print ( ANDROID_LOG_DEBUG, "VPLTEST", \
        "### %s:%d:" fmt,  __FILE__, __LINE__, ##__VA_ARGS__)

#else
#  define VPLTEST_PRINT(fmt, ...)  printf("### %s:%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#  define VPLTEST_LOG(fmt, ...)  printf("### %s:%d:"fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

#  define VPLTEST_DEBUG(fmt, ...)  printf("--- %s:%d:"fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

/// You should usually use #VPLTEST_FAIL or #VPLTEST_NONFATAL_ERROR instead of this, since we want
/// to increment the error count as well.
#ifdef VPL_PLAT_IS_WINRT
#  define VPLTEST_LOG_ERR(fmt, ...)  \
    VPLTEST_LOGWRITE( "*** %s:%d:"fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__); 
#elif defined(ANDROID)
#  define VPLTEST_LOG_ERR(fmt, ...) __android_log_print ( ANDROID_LOG_DEBUG, "VPLTEST", \
        "*** %s:%d:"fmt"\n",  __FILE__, __LINE__, ##__VA_ARGS__)
#else
#  define VPLTEST_LOG_ERR(fmt, ...)  printf("*** %s:%d:"fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif
/// @deprecated #VPLTEST_LOG_ERR automatically adds newlines.
#ifdef VPL_PLAT_IS_WINRT
#  define VPLTEST_NOTICE(...) \
    VPLTEST_LOGWRITE( __VA_ARGS__ );
#elif defined(ANDROID)
#  define VPLTEST_NOTICE(...) __android_log_print ( ANDROID_LOG_DEBUG, "VPLTEST", "*** " __VA_ARGS__)
#else
#  define VPLTEST_NOTICE(...)  printf("*** " __VA_ARGS__)
#endif
/// @deprecated
#define APP_LOG  VPLTEST_PRINT
/// @deprecated
#define APP_NOTICE  VPLTEST_NOTICE

//----------------------------------------------------------------------------

extern jmp_buf vplTestErrExit;

/// Sets the location to jump to when aborting a test.
/// @return #VPL_TRUE if this is the initial invocation.
#define vplTest_setjmp()  (setjmp(vplTestErrExit) == 0)

/// Logs an error, increments the error count, and aborts the test (returning to
/// the location where #vplTest_setjmp() was previously called).
void vplTest_fail(char const* file, char const* func, int line);

//----------------------------------------------------------------------------

/// Logs an error, increments the error count, and aborts the test (returning to
/// the location where #vplTest_setjmp() was previously called).
#define VPLTEST_FAIL(fmt_, ...)  VPLTEST_LOG_ERR("FAILED ENSURE: "fmt_, ##__VA_ARGS__); vplTest_fail(__FILE__, __func__, __LINE__)

/// Logs an error and increments the error count (the test keeps running).
#define VPLTEST_NONFATAL_ERROR(fmt_, ...)  VPLTEST_LOG_ERR("FAILED CHECK: "fmt_, ##__VA_ARGS__); vplTest_incrErrCount()

//----------------------------------------------------------------------------
/// @name VPLTEST_ENSURE_*
//@{
/// All VPLTEST_ENSURE_* macros log an error, increment the error count, and abort the
/// test if the specified condition is not satisfied.
/// @hideinitializer

#define VPLTEST_ENSURE_OK(rc, detailmsg_, ...) \
    VPLTEST_ENSURE_EQ_VARG((rc), VPL_OK, FMT_VPL_ERR, ARG_VPL_ERR, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_NONNEGATIVE(rc, detailmsg_, ...) \
    VPLTEST_ENSURE_GREATER_OR_EQ_VARG((rc), VPL_OK, FMT_VPL_ERR, ARG_VPL_ERR, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_NOT_NULL(actual, detailmsg_, ...) \
    VPLTEST_ENSURE_NOT_EQ_VARG((actual), NULL, FMT0xPTR, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_EQ_MEM(actual, actualLen, expected, expectedLen, detailmsg_, ...) \
    _VPLTEST_HELPER_MEMCMP(VPLTEST_FAIL, (actual), (actualLen), (expected), (expectedLen), detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_EQ_STRING(actual, expected, detailmsg_, ...) \
    _VPLTEST_HELPER_STRCMP(VPLTEST_FAIL, (actual), (expected), detailmsg_, ##__VA_ARGS__)

//---------------------

#define VPLTEST_ENSURE_EQUAL(actual, expected, format_str, detailmsg_, ...) \
    VPLTEST_ENSURE_EQ_VARG((actual), (expected), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_NOT_EQUAL(actual, bad_val, format_str, detailmsg_, ...) \
    VPLTEST_ENSURE_NOT_EQ_VARG((actual), (bad_val), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_LESS_OR_EQ(actual, expected, format_str, detailmsg_, ...) \
    VPLTEST_ENSURE_LESS_OR_EQ_VARG((actual), (expected), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_LESS(actual, expected, format_str, detailmsg_, ...) \
    VPLTEST_ENSURE_LESS_VARG((actual), (expected), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_GREATER_OR_EQ(actual, expected, format_str, detailmsg_, ...) \
    VPLTEST_ENSURE_GREATER_OR_EQ_VARG((actual), (expected), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_GREATER(actual, expected, format_str, detailmsg_, ...) \
    VPLTEST_ENSURE_GREATER_VARG((actual), (expected), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

//---------------------

#define VPLTEST_ENSURE_EQ_VARG(actual, expected, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_EQ(VPLTEST_FAIL, (actual), (expected), format_str, formatter, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_NOT_EQ_VARG(actual, bad_val, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_NOT_EQ(VPLTEST_FAIL, (actual), (bad_val), format_str, formatter, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_LESS_OR_EQ_VARG(actual, expected, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_ANY_OP(VPLTEST_FAIL, <=, (actual), (expected), format_str, formatter, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_LESS_VARG(actual, expected, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_ANY_OP(VPLTEST_FAIL, <, (actual), (expected), format_str, formatter, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_GREATER_OR_EQ_VARG(actual, expected, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_ANY_OP(VPLTEST_FAIL, >=, (actual), (expected), format_str, formatter, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_ENSURE_GREATER_VARG(actual, expected, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_ANY_OP(VPLTEST_FAIL, >, (actual), (expected), format_str, formatter, detailmsg_, ##__VA_ARGS__)

//@}

//----------------------------------------------------------------------------
///@name VPLTEST_CHK_*
//@{
/// All VPLTEST_CHK_* macros log an error and increment the error count if the
/// specified condition is not satisfied (the test keeps running).
/// @hideinitializer

#define VPLTEST_CHK_OK(rc, detailmsg_, ...) \
    VPLTEST_CHK_EQ_VARG((rc), VPL_OK, FMT_VPL_ERR, ARG_VPL_ERR, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_NONNEGATIVE(rc, detailmsg_, ...) \
    VPLTEST_CHK_GREATER_OR_EQ_VARG((rc), VPL_OK, FMT_VPL_ERR, ARG_VPL_ERR, detailmsg_, ##__VA_ARGS__)

// Check that the return value matches (interpreted as VPL error code).
#define VPLTEST_CHK_RV(rc, expected, detailmsg_, ...) \
    VPLTEST_CHK_EQ_VARG((rc), expected, FMT_VPL_ERR, ARG_VPL_ERR, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_NOT_NULL(actual, detailmsg_, ...) \
    VPLTEST_CHK_NOT_EQ_VARG((actual), NULL, FMT0xPTR, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_EQ_MEM(actual, actualLen, expected, expectedLen, detailmsg_, ...) \
    _VPLTEST_HELPER_MEMCMP(VPLTEST_NONFATAL_ERROR, (actual), (actualLen), (expected), (expectedLen), detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_EQ_STRING(actual, expected, detailmsg_, ...) \
    _VPLTEST_HELPER_STRCMP(VPLTEST_NONFATAL_ERROR, (actual), (expected), detailmsg_, ##__VA_ARGS__)

//---------------------

#define VPLTEST_CHK_EQUAL(actual, expected, format_str, detailmsg_, ...) \
    VPLTEST_CHK_EQ_VARG((actual), (expected), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_NOT_EQUAL(actual, bad_val, format_str, detailmsg_, ...) \
    VPLTEST_CHK_NOT_EQ_VARG((actual), (bad_val), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_LESS_OR_EQ(actual, expected, format_str, detailmsg_, ...) \
    VPLTEST_CHK_LESS_OR_EQ_VARG((actual), (expected), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_LESS(actual, expected, format_str, detailmsg_, ...) \
    VPLTEST_CHK_LESS_VARG((actual), (expected), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_GREATER_OR_EQ(actual, expected, format_str, detailmsg_, ...) \
    VPLTEST_CHK_GREATER_OR_EQ_VARG((actual), (expected), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_GREATER(actual, expected, format_str, detailmsg_, ...) \
    VPLTEST_CHK_GREATER_VARG((actual), (expected), format_str, /*no formatter*/, detailmsg_, ##__VA_ARGS__)

//---------------------

#define VPLTEST_CHK_EQ_VARG(actual, expected, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_EQ(VPLTEST_NONFATAL_ERROR, (actual), (expected), format_str, formatter, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_NOT_EQ_VARG(actual, bad_val, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_NOT_EQ(VPLTEST_NONFATAL_ERROR, (actual), (bad_val), format_str, formatter, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_LESS_OR_EQ_VARG(actual, expected, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_ANY_OP(VPLTEST_NONFATAL_ERROR, <=, (actual), (expected), format_str, formatter, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_LESS_VARG(actual, expected, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_ANY_OP(VPLTEST_NONFATAL_ERROR, <, (actual), (expected), format_str, formatter, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_GREATER_OR_EQ_VARG(actual, expected, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_ANY_OP(VPLTEST_NONFATAL_ERROR, >=, (actual), (expected), format_str, formatter, detailmsg_, ##__VA_ARGS__)

#define VPLTEST_CHK_GREATER_VARG(actual, expected, format_str, formatter, detailmsg_, ...) \
    _VPLTEST_HELPER_ANY_OP(VPLTEST_NONFATAL_ERROR, >, (actual), (expected), format_str, formatter, detailmsg_, ##__VA_ARGS__)

//@}

//----------------------------------------------------------------------------

/// Call a function and expect a particular int (interpreted as VPL error code).
#define VPLTEST_CALL_AND_CHK_RV(call, expected, ...) \
    _VPLTEST_HELPER_EXPECT(VPLTEST_NONFATAL_ERROR, int, FMT_VPL_ERR, ARG_VPL_ERR, (call), (expected), #call" "__VA_ARGS__)

/// Call a function and expect a particular int.
#define VPLTEST_CALL_AND_CHK_INT(call, expected, ...) \
    _VPLTEST_HELPER_EXPECT(VPLTEST_NONFATAL_ERROR, s64, FMTs64, /*no formatter*/, (call), (expected), #call" "__VA_ARGS__)

/// Call a function and expect a particular unsigned int.
#define VPLTEST_CALL_AND_CHK_UNSIGNED(call, expected, ...) \
    _VPLTEST_HELPER_EXPECT(VPLTEST_NONFATAL_ERROR, u64, FMTu64, /*no formatter*/, (call), (expected), #call" "__VA_ARGS__)

//---------------------

/// Call a function and abort the test if the result is not #VPL_OK.
#define VPLTEST_CALL_AND_ENSURE_OK(call, ...) \
    _VPLTEST_HELPER_EXPECT(VPLTEST_NONFATAL_ERROR, int, FMT_VPL_ERR, ARG_VPL_ERR, (call), VPL_OK, #call" "__VA_ARGS__)

//----------------------------------------------------------------------------

#define _VPLTEST_HELPER_EQ(severity, actual, expected, format_str, formatter, detail_msg, ...) \
    if ((actual) != (expected)) { \
        severity( \
                "expected <"format_str">, got <"format_str">: "detail_msg, \
                formatter(expected), formatter(actual), ##__VA_ARGS__); \
    } else { \
        VPLTEST_DEBUG("Got <"format_str"> as expected ("detail_msg")", \
                formatter(actual), ##__VA_ARGS__); \
    }

#define _VPLTEST_HELPER_NOT_EQ(severity, actual, bad_val, format_str, formatter, detail_msg, ...) \
    if ((actual) == (bad_val)) { \
        severity( \
                "unexpected <"format_str">: "detail_msg, \
                formatter(actual), ##__VA_ARGS__); \
    } else { \
        VPLTEST_DEBUG("As expected, \""format_str"\" was != \""format_str"\" ("detail_msg")", \
                formatter(actual), formatter(bad_val), ##__VA_ARGS__); \
    }

#define _VPLTEST_HELPER_ANY_OP(severity, operation, actual, expected, format_str, formatter, detail_msg, ...) \
    if (!((actual) operation (expected))) { \
        severity( \
                "\""format_str"\" should have been "#operation" \""format_str"\": "detail_msg, \
                formatter(actual), formatter(expected), ##__VA_ARGS__); \
    } else { \
        VPLTEST_DEBUG("As expected, \""format_str"\" was "#operation" \""format_str"\" ("detail_msg")", \
                formatter(actual), formatter(expected), ##__VA_ARGS__); \
    }

#define _VPLTEST_HELPER_STRCMP(severity, actual, expected, detail_msg, ...) \
    if (strcmp((expected), (actual)) != 0) { \
        severity( \
                "expected <%s>, got <%s>: "detail_msg, \
                (expected), (actual), ##__VA_ARGS__); \
    }

#define _VPLTEST_HELPER_MEMCMP(severity, actual, actualLen, expected, expectedLen, detail_msg, ...) \
    if ((actualLen) != (expectedLen)) { \
        severity( \
                "Buffer length wrong: expected <"FMTu64">, got <"FMTu64">: "detail_msg, \
                (u64)(expectedLen), (u64)(actualLen), ##__VA_ARGS__); \
    } else { \
        if (memcmp((expected), (actual), (expectedLen)) != 0) { \
            severity( \
                    "Buffer has correct length, but wrong content: "detail_msg, \
                    ##__VA_ARGS__); \
        } \
    }

#define _VPLTEST_HELPER_EXPECT(severity, return_type, format_str, formatter, call, expected, ...) \
    BEGIN_MULTI_STATEMENT_MACRO \
    return_type vpltest_helper_actual = (call); \
    return_type vpltest_helper_expected = (expected); \
    _VPLTEST_HELPER_EQ(severity, vpltest_helper_actual, vpltest_helper_expected, format_str, formatter, __VA_ARGS__); \
    END_MULTI_STATEMENT_MACRO

//----------------------------------------------------------------------------

int vplTest_getErrCount(void);

int vplTest_getTotalErrCount(void);

/// Increments the error count; it is generally preferable to call
/// #VPLTEST_NONFATAL_ERROR instead of this, since we want to log the error too.
void vplTest_incrErrCount(void);

#ifdef __cplusplus
}
#endif

#endif // include guard
