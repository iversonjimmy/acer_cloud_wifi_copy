//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLU_COMMON_H__
#define __VPLU_COMMON_H__

//============================================================================
/// @file
/// VPL utility (VPLU) definitions with no dependencies.
//============================================================================

/// For acknowledging "unused variable" warnings.
#ifndef UNUSED
#define UNUSED(x)  (void)(x)
#endif

/// For acknowledging "unused return value" warnings.
#ifndef IGNORE_RESULT
#define IGNORE_RESULT  void
#endif

/// Do nothing. Useful for putting a goto label at the end of a block.
#ifndef NO_OP
#define NO_OP()  ((void)0)
#endif

#ifdef __cplusplus

/// Fail at compile-time if provided a non-array variable.
template <typename T, int N> T (&arrayTest(T(&)[N]))[N];
template <typename T, int N> char (&arrayAsChars(T(&)[N]))[N];

/// Makes sure that 'array' is really an array at compile-time.
#define ARRAY_SIZE_IN_BYTES(array)  (sizeof(arrayTest(array)))

/// Gets the number of elements and makes sure that 'array' is really an array at compile-time.
#define ARRAY_ELEMENT_COUNT(array)  (sizeof(arrayAsChars(array)))

#else

#define ARRAY_SIZE_IN_BYTES(array)  (sizeof(array))

/// The number of elements in the array.
#define ARRAY_ELEMENT_COUNT(array)  (sizeof(array) / sizeof(array[0]))

#endif

/// Get the compile-time size of a field within a struct/union type.
/// Typical sizeof() doesn't let you refer to a field within a type (you need an
/// instance of that type).
#define SIZEOF_FIELD(type, field)  (sizeof(((type*)0)->field))

//#ifndef MIN
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

/// A definition of "NULL" that is always a pointer.
/// In C++, NULL is normally just "0", but gcc doesn't consider that a pointer for
/// purposes of checking printf function parameters.
#define VPL_VOID_PTR_NULL ((void*)(0))

/// This only applies for ranges that start at zero and don't skip any integers.
#define VPL_IS_UNSIGNED_INDEX_IN_RANGE(index, num_values) \
    ((unsigned)(index) < (num_values))

/// Turn @a X into a string by surrounding it with quotes.
/// If @a X is a macro, it will be expanded first.
#define VPL_STRING(X)  VPL_STRING_NO_EXPAND(X)

/// Turn @a X into a string by surrounding it with quotes.
/// If @a X is a macro, it will *not* be expanded.
#define VPL_STRING_NO_EXPAND(X)  #X

/// Use to avoid "conditional expression is constant" warning when you want to
/// specify something like \c "while(0)".
#define ALWAYS_FALSE_CONDITIONAL  (__LINE__ == -1)

/// For making multiple-statement macros less dangerous.
#define BEGIN_MULTI_STATEMENT_MACRO  do {

/// For making multiple-statement macros less dangerous.
#define END_MULTI_STATEMENT_MACRO  } while(ALWAYS_FALSE_CONDITIONAL)

/// Use in the "private" section of a class to prevent accidents.
#define VPL_DISABLE_COPY_AND_ASSIGN(ClassName) \
    ClassName(const ClassName&); \
    void operator=(const ClassName&)

//============================================================================
// Compiler attributes

/// Indicates that some of the function's parameters should never be NULL.
/// If any calls to the function are guaranteed to pass a NULL argument, the
/// compiler should emit a warning at compile-time.
#define VPL_ATTR_NONNULL  ATTRIBUTE_NONNULL

/// Indicates that a function should not be provided any NULL parameters.
/// If any calls to the function are guaranteed to pass a NULL argument, the
/// compiler should emit a warning at compile-time.
#define VPL_NO_NULL_ARGS  NO_NULL_ARGS

/// Indicates that a function behaves like printf.  The compiler should verify
/// (at compile-time) that the variable-length arguments to the function match
/// the format string.
/// fmtIdx and vaIdx are 1-based (the first parameter is idx 1, not 0).
/// To allow a null format string, use #ATTRIBUTE_NULLABLE_PRINTF instead.
#define VPL_ATTR_PRINTF  ATTRIBUTE_PRINTF

/// Same as #VPL_ATTR_PRINTF, but the format string can be null.
#define VPL_ATTRIBUTE_NULLABLE_PRINTF  ATTRIBUTE_NULLABLE_PRINTF

#ifdef _MSC_VER
#  define VPL_THREAD_LOCAL  __declspec(thread)
#elif defined(ANDROID) 
#  define VPL_THREAD_LOCAL  ERROR_ANDROID_DOES_NOT_SUPPORT_THREAD_LOCAL_KEYWORD
#else
#  define VPL_THREAD_LOCAL  __thread
#endif

//=====================

#define VPL_COMPUTE_GCC_VERSION(major, minor, patch) \
    (((((major) * 100) + minor)*10000) + patch)

#ifdef __GNUC__
#   define VPL_GCC_VERSION_NUMBER  VPL_COMPUTE_GCC_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
#   define VPL_GCC_VERSION_NUMBER  0
#endif

/// If supported, allow the compiler to detect unexpected NULL arguments.
#if (VPL_GCC_VERSION_NUMBER >= VPL_COMPUTE_GCC_VERSION(3, 3, 0))
#   define ATTRIBUTE_NONNULL(...)  __attribute__((nonnull (__VA_ARGS__)))
#   define NO_NULL_ARGS  __attribute__((nonnull))
#else
#   define ATTRIBUTE_NONNULL(...)
#   define NO_NULL_ARGS
#endif

//% TODO: Can we validate this with static analysis?  I want this to catch bugs.
//% Optimization is just a bonus.
//% #if (VPL_GCC_VERSION_NUMBER >= VPL_COMPUTE_GCC_VERSION(3, 0, 0)) && 0
//%    #define ATTRIBUTE_PURE  __attribute__ ((pure))
#define ATTRIBUTE_PURE

/// Platforms that have specific alignment requirements will define
/// ATTRIBUTE_ALIGN(x) to allow static buffers to be declared with proper
/// alignment. Those that don't will define the macro as nothing.
/// @note This is guaranteed to work for static globals, but whether or not it
///    works for local variables on the stack may vary by platform.
//% Should work for local variables on BW; see wiki [[Broadway Alignment Notes]]
#ifndef ATTRIBUTE_ALIGN
#define ATTRIBUTE_ALIGN(x)
#endif

/// If supported, allow the compiler to verify printf formatting strings passed to our functions.
/// fmtIdx and vaIdx are 1-based (the first parameter is idx 1, not 0).
/// To allow a null format string, use #ATTRIBUTE_NULLABLE_PRINTF instead.
#if (VPL_GCC_VERSION_NUMBER >= VPL_COMPUTE_GCC_VERSION(3, 0, 0))
#   define ATTRIBUTE_PRINTF(fmtIdx, vaIdx) \
        __attribute__((format (printf, fmtIdx, vaIdx))) ATTRIBUTE_NONNULL(fmtIdx)
#else
#   define ATTRIBUTE_PRINTF(fmtIdx, vaIdx)
#endif

/// If supported, allow the compiler to verify scanf formatting strings passed to our functions.
/// fmtIdx and vaIdx are 1-based (the first parameter is idx 1, not 0).
#if (VPL_GCC_VERSION_NUMBER >= VPL_COMPUTE_GCC_VERSION(3, 0, 0))
#   define ATTRIBUTE_SCANF(fmtIdx, vaIdx) \
        __attribute__((format (scanf, fmtIdx, vaIdx))) ATTRIBUTE_NONNULL(fmtIdx)
#else
#   define ATTRIBUTE_SCANF(fmtIdx, vaIdx)
#endif

/// The behavior changed in 3.3; before that, "format" implied "nonnull" semantics.
#if (VPL_GCC_VERSION_NUMBER >= VPL_COMPUTE_GCC_VERSION(3, 3, 0))
#   define ATTRIBUTE_NULLABLE_PRINTF(fmtIdx, vaIdx) \
        __attribute__((format (printf, fmtIdx, vaIdx)))
#else
#   define ATTRIBUTE_NULLABLE_PRINTF(fmtIdx, vaIdx)
#endif

//============================================================================

#endif // include guard
