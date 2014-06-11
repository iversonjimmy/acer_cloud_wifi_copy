//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_ASSERT_H__
#define __VPLEX_ASSERT_H__

#include "vpl_plat.h"
#include "vplex_plat.h"
#include "vpl_types.h"
#include "vpl_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef VPL_NO_ASSERT
#   define VPLAssert_Failed(file, func, line, ...) ((void)0)
#   define VPLAssert_FailedOrNull  (NULL)
    
#   ifndef ASSERT
#   define ASSERT(exp)  ((void)0)
#   endif

#   ifndef ASSERTMSG
#   define ASSERTMSG(exp, ...)  ((void)0)
#   endif

#else // !VPL_NO_ASSERT

    //% TODO: We should try making our assert handler a weak symbol so that users can override it at link time.
    //% __declspec(weak)
    /// Display the message and code location and freeze the process. During debugging, it can be
    /// useful if execution is allowed to resume after calling this function.
    /// #FAILED_ASSERT should generally be used instead of this, but this version is provided to
    /// allow functions to report the location of the initial call site instead of the
    /// location in the called function's definition.
    void VPLAssert_Failed(char const* file, char const* func, int line, char const* formatMsg, ...)
            ATTRIBUTE_PRINTF(4, 5);

#   define VPLAssert_FailedOrNull  (VPLAssert_Failed)

#   ifndef ASSERT
#   define ASSERT(exp)  (void)((exp) || (FAILED_ASSERT(FMTstr, #exp), 0))
#   endif

#   ifndef ASSERTMSG
#   define ASSERTMSG(exp, ...)  (void)((exp) || (FAILED_ASSERT(__VA_ARGS__), 0))
#   endif

#endif // !VPL_NO_ASSERT

#ifndef FAILED_ASSERT
#define FAILED_ASSERT(...)  \
    VPLAssert_Failed(_FILE_NO_DIRS_, __func__, __LINE__, __VA_ARGS__)
#endif

#ifndef FAILED_ASSERT_NO_MSG
#define FAILED_ASSERT_NO_MSG()  \
    VPLAssert_Failed(_FILE_NO_DIRS_, __func__, __LINE__, "Unexpected!")
#endif

#define ASSERT_OP(operation, actual, expected, strActual, strExpected, format) \
    ASSERTMSG((actual) operation (expected), "<"strActual"> ("format") "#operation" <"strExpected"> ("format")", actual, expected)

#define ASSERT_OP_VARG(operation, actual, expected, strActual, strExpected, format, vargName) \
    ASSERTMSG((actual) operation (expected), "<"strActual"> ("format") "#operation" <"strExpected"> ("format")", vargName(actual), vargName(expected))

#define _ASSERT_FUNC_TRUE(functionName, actual, expected, strActual, strExpected, format) \
    ASSERTMSG(functionName(&(actual), &(expected)), #functionName": <"strActual"> ("format"), <"strExpected"> ("format")", actual, expected)
#define _ASSERT_FUNC_TRUE_VARG(functionName, actual, expected, strActual, strExpected, format, vargName) \
    ASSERTMSG(functionName(&(actual), &(expected)), #functionName": <"strActual"> ("format"), <"strExpected"> ("format")", vargName(actual), vargName(expected))

#define ASSERT_EQUAL_COMPARATOR(functionName, actual, expected, format) \
    _ASSERT_FUNC_TRUE(functionName, (actual), (expected), #actual, #expected, format)
#define ASSERT_EQUAL_COMPARATOR_VARG(functionName, actual, expected, format, vargName) \
    _ASSERT_FUNC_TRUE_VARG(functionName, (actual), (expected), #actual, #expected, format, vargName)

#define ASSERT_EQUAL(actual, expected, format) \
    ASSERT_OP(==, actual, expected, #actual, #expected, format)
#define ASSERT_EQUAL_VARG(actual, expected, format, vargName) \
    ASSERT_OP_VARG(==, actual, expected, #actual, #expected, format, vargName)

#define ASSERT_NOT_EQUAL(actual, expected, format) \
    ASSERT_OP(!=, actual, expected, #actual, #expected, format)
#define ASSERT_NOT_EQUAL_VARG(actual, expected, format, vargName) \
    ASSERT_OP_VARG(!=, actual, expected, #actual, #expected, format, vargName)

#define ASSERT_GREATER_OR_EQ(actual, expected, format) \
    ASSERT_OP(>=, (actual), expected, #actual, #expected, format)
#define ASSERT_GREATER_OR_EQ_VARG(actual, expected, format, vargName) \
    ASSERT_OP_VARG(>=, (actual), expected, #actual, #expected, format, vargName)

#define ASSERT_GREATER(actual, expected, format) \
    ASSERT_OP(>, (actual), expected, #actual, #expected, format)
#define ASSERT_GREATER_VARG(actual, expected, format, vargName) \
    ASSERT_OP_VARG(>, (actual), expected, #actual, #expected, format, vargName)

#define ASSERT_LESS_OR_EQ(actual, expected, format) \
    ASSERT_OP(<=, (actual), expected, #actual, #expected, format)
#define ASSERT_LESS_OR_EQ_VARG(actual, expected, format, vargName) \
    ASSERT_OP_VARG(<=, (actual), expected, #actual, #expected, format, vargName)

#define ASSERT_LESS(actual, expected, format) \
    ASSERT_OP(<, (actual), expected, #actual, #expected, format)
#define ASSERT_LESS_VARG(actual, expected, format, vargName) \
    ASSERT_OP_VARG(<, (actual), expected, #actual, #expected, format, vargName)

#define ASSERT_IS_NULL(ptr)  ASSERT_EQUAL(ptr, VPL_VOID_PTR_NULL, "%p")

#define ASSERT_NOT_NULL(ptr)  ASSERT_NOT_EQUAL(ptr, VPL_VOID_PTR_NULL, "%p")

#define ASSERT_VPL_OK(errCode)  ASSERT_EQUAL(errCode, VPL_OK, "%d");

/// Mark locations that should never be reached and can be verified via simple control flow analysis.
// TODO: how to get the compiler to verify this instead of complaining?
#define ASSERT_STATICALLY_UNREACHABLE()

/// A nicer-looking alias for #FAILED_ASSERT_NO_MSG.
#define ASSERT_UNREACHABLE  FAILED_ASSERT_NO_MSG

/// This only applies for ranges that start at zero and don't skip any numbers.
#define ASSERT_UNSIGNED_INDEX_IN_RANGE(actual, num_values) \
    ASSERT_LESS((unsigned)(actual), (unsigned)(num_values), "%u")

/// Useful for the default case in a switch.
#define UNHANDLED_ENUM_PANIC(value)  \
    FAILED_ASSERT("Unknown enum value <%d>", (int)(value));

/// Should theoretically be able to validate these assertions at compile-time.
// TODO: Use some C++ template magic if possible.
#define ASSERT_EQUAL_SIZE_COMPILE_TIME(actual, expected)  ASSERT_EQUAL((actual), (expected), FMTu_size_t)

#ifdef  __cplusplus
}
#endif

#endif // include guard
