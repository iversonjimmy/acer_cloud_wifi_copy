//
//  Copyright 2010-2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_THREAD_H__
#define __VPL_THREAD_H__

//============================================================================
/// @file
/// Virtual Platform Layer API for creating and managing threads.
/// @see VPLThreads
//============================================================================

#include "vpl_plat.h"
#include "vpl_time.h"

#include <stdlib.h>  // for size_t
#include <stddef.h> // for ptrdiff_t

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLThreads VPL Thread API
///
/// VPL functionality for creating and manipulating threads.
///
/// The VPLThread API is designed to work much like POSIX threads (pthreads).
/// New threads are created via #VPLThread_Create(). To create a thread
/// with a specific thread priority or stack size, you should first initialize a
/// #VPLThread_attr_t via #VPLThread_AttrInit() and then configure it via
/// #VPLThread_AttrSetPriority() and #VPLThread_AttrSetStackSize() before
/// passing it to #VPLThread_Create().
///
/// A thread terminates when it returns from its start routine (#VPLThread_fn_t)
/// or when it calls #VPLThread_Exit().
///
/// To synchronize threads, VPL provides \ref VPLThreadSynchronization.
///@{

//% TODO: where should this go in the generated documentation?
// Some types have PRI_<type> and FMT_<type> macros for use with printf-type
// operations.
// Use PRI_<type> for the format code.
// Example: printf("Print type foo %"PRI_foo".", var_foo);
// Use FMT_<type> for the complete format string for the variable.
// Example: printf("Print type foo "FMT_foo".", var_foo);
// Some types also have VAL_<type>. When present, use these instead of the
// variable.
// Example: printf("Print complex type bar "FMT_bar".", VAL(var_bar));

/// Return type of the start routine passed to #VPLThread_Create().
typedef void* VPLThread_return_t;

/// Casts the value to #VPLThread_return_t without triggering conversion warnings.
#define VPL_AS_THREAD_RETVAL(value)  ((VPLThread_return_t)((ptrdiff_t)(value)))
#define VPL_AS_THREAD_RETVAL_PTR(value)  ((VPLThread_return_t*)((ptrdiff_t)(value)))

//% TODO: this is more of a utility definition; I'd move it out of the main API.
/// Indicates that the function is only returning this value to
/// satisfy the #VPLThread_fn_t signature; the value should not
/// actually be used.
#define VPLTHREAD_RETURN_VALUE_UNUSED  VPL_NULL

/// Function argument type for the start routine passed to #VPLThread_Create().
typedef void* VPLThread_arg_t;
/// Casts the value to #VPLThread_arg_t without triggering conversion warnings.
#define VPL_AS_THREAD_FUNC_ARG(value)  ((VPLThread_arg_t)((ptrdiff_t)(value)))
/// Casts the #VPLThread_arg_t without triggering conversion warnings.
#define VPLTHREAD_FUNC_ARG_TO_INT(value)  ((int)((ptrdiff_t)(value)))
/// Casts the #VPLThread_arg_t without triggering conversion warnings.
#define VPLTHREAD_FUNC_ARG_TO_UINT(value)  ((unsigned int)((ptrdiff_t)(value)))

/// Type of the start routine passed to #VPLThread_Create().
typedef VPLThread_return_t (*VPLThread_fn_t)(VPLThread_arg_t);

/// @name VPLThreadPriorities
///
/// VPL Thread Priorities.
/// A VPL thread priority is some integer between VPL_PRIO_MIN and VPL_PRIO_MAX.
/// The priority level will be mapped to the appropriate platform-specific
/// values at runtime. Higher numerical values have higher relative priority.
/// VPL_PRIO_MIN maps to the default POSIX thread priority.
//@{

#define VPL_PRIO_MIN (0)
#define VPL_PRIO_MAX (9)

/// An alias for #VPL_PRIO_MIN.
#define VPL_PRIO_IDLE (VPL_PRIO_MIN)

//% close named group VPLThreadPriorities
//@}

// Platform specific definitions.
#include "vpl__th.h"

// TODO: bug 513: rename:
// VPLThread_t -> VPLLegacyThread_t
// VPLThread__t -> VPLLegacyThread__t
// VPLTHREAD_INVALID -> VPLLEGACYTHREAD_INVALID
// VPLThread_Create -> VPLLegacyThread_Create
// VPLThread_fn_t -> VPLLegacyThread_fn_t
// VPLThread_return_t -> VPLLegacyThread_return_t
// VPLThread_Exit -> VPLLegacyThread_Exit
// VPLThread_Join -> VPLLegacyThread_Join
// VPLThread_Self -> VPLLegacyThread_Self
// VPLThread_Equal -> VPLLegacyThread_Equal
// VPLDetachableThread* -> VPLThread*

/// VPL thread type.
/// It is important to keep this variable in scope until the thread exits.
/// @deprecated Please use #VPLDetachableThread instead.
typedef VPLThread__t VPLThread_t;

/// Constant #VPLThread_t value, for initializing #VPLThread_t variables.
/// @deprecated This shouldn't be needed for new code.
extern const VPLThread_t VPLTHREAD_INVALID;

/// Maximum number of concurrent #VPLThread_t objects.
#define VPLTHREAD_MAX_THREADS    VPLTHREAD__MAX_THREADS

/// Maximum number of concurrent #VPLSem_t objects.
#define VPLTHREAD_MAX_SEMAPHORES  VPLTHREAD__MAX_SEMAPHORES

/// Maximum concurrent number of #VPLMutex_t objects.
#define VPLTHREAD_MAX_MUTEXES    VPLTHREAD__MAX_MUTEXES

/// Maximum concurrent number of #VPLCond_t objects
#define VPLTHREAD_MAX_CONDVARS   VPLTHREAD__MAX_CONDVARS

/// @name VPLThreadPrintfDirectives
//@{

/// <code>printf()</code> format directive to print a #VPLThread_t.
/// For example, String-paste #FMT_VPLThread_t into printf()'s first argument.
#define PRI_VPLThread_t  PRI_VPLThread__t
#define FMT_VPLThread_t  "%"PRI_VPLThread_t

/// Macro to render a #VPLThread_t object, into the format expected by #FMT_VPLThread_t.
#define VAL_VPLThread_t  VAL_VPLThread__t

//@}

/// @name VPLThreadCreationAttributes
/// Controlling Initial Thread Resource Consumption
///
/// Thread attributes provide control of per-thread resources at thread-creation time.
/// VPL programmers must threat #VPLThread_attr_t as an opaque type.
///
/// Objects of type #VPLThread_attr_t must be initialized by calling
/// #VPLThread_AttrInit() before being passed to any other VPLThread functions.
/// Once initialized, thread attributes may be modified by \c VPLThread_AttrSet*() calls
/// and retrieved via \c VPLThread_AttrGet*() calls.
///
/// Initial stack-size of a VPL thread can be set via #VPLThread_AttrSetStackSize().
/// The size of thread's stack is fixed at thread-creation time; once #VPLThread_Create()
/// succeeds, the stack-size of the resulting thread cannot be modified.
///
/// Initial priority of a VPL thread can be set via #VPLThread_AttrSetPriority().
///
//@{

/// Default stack-size for VPL threads.
#define VPLTHREAD_STACKSIZE_DEFAULT      VPLTHREAD__STACKSIZE_DEFAULT

/// Minimum supported stack-size for VPL threads.
#define VPLTHREAD_STACKSIZE_MIN          VPLTHREAD__STACKSIZE_MIN

/// Maximum supported stack-size for VPL threads.
#define VPLTHREAD_STACKSIZE_MAX          VPLTHREAD__STACKSIZE_MAX

/// Maximum POSIX thread priority for VPL threads.
#define VPLTHREAD_PTHREAD_PRIO_MAX       VPLTHREAD__PTHREAD_PRIO_MAX

/// Attributes used at thread-creation time to customize resources for a new thread.
typedef VPLThread__attr_t VPLThread_attr_t;

/// Initialize a #VPLThread_attr_t.
/// @param[in] attrs Address of the #VPLThread_attr_t to initialize.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID @a attrs is NULL.
int VPLThread_AttrInit(VPLThread_attr_t* attrs);

/// Destroy a Thread-Attributes object.
/// Once no more threads will be created using the #VPLThread_attr_t,
/// it should be destroyed by #VPLThread_AttrDestroy().
/// Calling #VPLThread_AttrDestroy() has no effect on threads created
/// with that #VPLThread_attr_t.
/// Once #VPLThread_AttrDestroy() succeeds, the #VPLThread_attr_t may then be
/// reinitialized by #VPLThread_AttrInit() and re-used to create more new threads.
/// @param[in] attrs Address of the #VPLThread_attr_t to initialize.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID @a attrs is NULL.
int VPLThread_AttrDestroy(VPLThread_attr_t* attrs);

// Accessors/Mutators (setters/getters)

/// Set Thread Stack-Size setting to a #VPLThread_attr_t.
/// @param[in] attrs VPL Thread-attributes object.
/// @param[in] stackSize Stack size, in bytes.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID @a attrs is NULL, or @a stackSize is out-of-range.
int VPLThread_AttrSetStackSize(VPLThread_attr_t* attrs, size_t stackSize);

/// Get Thread Stack-Size setting from a #VPLThread_attr_t.
/// @param[in] attrs VPL Thread-attributes object.
/// @param[out] stackSize Stack size, in bytes.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID @a attrs or @a stackSize is NULL.
int VPLThread_AttrGetStackSize(const VPLThread_attr_t* attrs, size_t* stackSize_out);

/// Set Thread Priority to a #VPLThread_attr_t.
/// @param[in] attrs VPL Thread-attributes object.
/// @param[in] vplPrio
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID @a attrs is NULL, or @a vplPrio is invalid.
int VPLThread_AttrSetPriority(VPLThread_attr_t* attrs, int vplPrio);

/// Get Thread Priority from a #VPLThread_attr_t.
/// @param[in] attrs VPL Thread-attributes object.
/// @param[out] vplPrio
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID @a attrs or @a vplPrio is NULL.
int VPLThread_AttrGetPriority(const VPLThread_attr_t* attrs, int* vplPrio_out);

/// Set Detach State to a #VPLThread_attr_t.
/// @param[in] attrs VPL Thread-attributes object.
/// @param[in] createDetached
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID @a attrs is NULL.
int VPLThread_AttrSetDetachState(VPLThread_attr_t* attrs, VPL_BOOL createDetached);

/// Get Detach State from a #VPLThread_attr_t.
/// @param[in] attrs VPL Thread-attributes object.
/// @param[out] createDetached
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID @a attrs or @a createDetached_out is NULL.
int VPLThread_AttrGetDetachState(const VPLThread_attr_t* attrs, VPL_BOOL* createDetached_out);

//% End of thread-attributes
//@}

/// NOTE: This is deprecated, please use #VPLDetachableThread_Create() instead.
/// Create and start a thread executing @a startRoutine with the given parameters.
/// @note The thread starts immediately. If the thread should wait for some
/// other condition before doing specific operations, use the #VPLCond_t
/// facilities to wait for the condition in @a startRoutine.
/// @param[in,out] thread The #VPLThread_t struct that will be used.  The VPLThread_t
///        must remain valid until the thread has exited, and should be
///        passed to #VPLThread_Join() to free thread resources.
/// @param[in] startRoutine Main routine for the thread.
/// @param[in] startArg Argument to pass to the thread's entrypoint, @a startRoutine.
/// @param[in] attrs A pointer to an initialized VPLThread_attr_t*, or NULL.
///            If NULL, #VPLThread_Create() will create a thread with "default" attributes:
///            the new thread inherits the priority of the calling thread,
///            and a thread stack-size set to a VPL-defined default value.
/// @param[in] threadName Name of the thread, NULL terminated.
/// @return #VPL_OK Success; thread started.
/// @return #VPL_ERR_MAX  The calling process has reached its limit of #VPLTHREAD_MAX_THREADS
///            concurrent threads.
/// @return #VPL_ERR_AGAIN Insufficient system resources to create the thread.
/// @return #VPL_ERR_INVALID @a thread or @a startRoutine are NULL or invalid.
/// @return #VPL_ERR_PERM The calling process does not have appropriate system permissions for this call.
int VPLThread_Create(VPLThread_t* thread,
                     VPLThread_fn_t startRoutine,
                     void* startArg,
                     const VPLThread_attr_t* attrs,
                     const char* threadName);

#ifndef VPL_PLAT_IS_WINRT
/// Terminate the calling thread.
/// Calling this function will terminate the calling thread.  @a retval will be
/// available to any successful #VPLThread_Join() with the terminated thread.
/// Any thread-specific data will have appropriate destructor functions
/// called in undefined order.  This function will not release process resources
/// such as #VPLMutex_t.
/// When the calling thread terminates, threads waiting in #VPLThread_Join()
/// for that thread to exit will be awoken and will return from #VPLThread_Join().
/// @param[out] retval Value to give to any thread that executes #VPLThread_Join()
///            as the "return value" for this thread.
void VPLThread_Exit(VPLThread_return_t retval);
#endif

/// NOTE: This is deprecated, please use #VPLDetachableThread_Join() instead.
/// Block until the indicated thread has exited.
/// This function will suspend execution of the calling thread until @a thread
/// terminates.  The value returned by the terminated thread (either by return value
/// of its #VPLThread_fn_t or by a call to #VPLThread_Exit()) will be available in
/// @a retvalOut.  Multiple simultaneous calls to #VPLThread_Join() on the same @a thread
/// is undefined.
/// @param[in] thread The thread for which to wait.
/// @param[out] retvalOut Location to store the return value of the exiting thread.  Can be NULL
///     if this value is not needed.
/// @return #VPL_OK if @a thread exited.
/// @return #VPL_ERR_INVALID @a thread is NULL or is not a joinable thread.
/// @return #VPL_ERR_DEADLK @a thread specifies the calling thread, so this would deadlock.
int VPLThread_Join(VPLThread_t* thread,
                   VPLThread_return_t* retvalOut);

/// Get a handle to that calling thread.
/// @return Thread ID of calling thread.
VPLThread_t VPLThread_Self(void);

/// Check whether two #VPLThread_t instances refer to the same thread.
/// @return 1 if threads are the same, else 0
/// @return #VPL_ERR_INVALID if t1 or t2 are NULL
int VPLThread_Equal(const VPLThread_t* t1, const VPLThread_t* t2);

/// Causes the calling thread to sleep for at least the specified time.
/// @param[in] time Number of microseconds to sleep.
/// @note VPLThread_Sleep(0) is equivalent to VPLThread_Yield().
void VPLThread_Sleep(VPLTime_t time);

/// Causes the calling thread to yield the processor, if other threads
/// are waiting to run.
void VPLThread_Yield(void);

/// Set the CPU affinity of the current thread.
/// Until this is called, the thread has no fixed CPU affinity.
/// Once this is called, the thread is locked to the specified CPU ID.
/// The CPU may be changed by calling again.
/// Set @a cpuId to -1 to remove CPU affinity (allow any CPU to be used).
/// @param[in] cpuId integer between 0 and #VPLThread_GetMaxValidCpuId()
/// @return #VPL_OK if successful
/// @return #VPL_ERR_INVALID @a cpuId is not a valid CPU id
/// @return #VPL_ERR_PERM The calling process does not have the appropriate privileges.
/// @note CPU affinity is inherited from the parent process initially.
int VPLThread_SetAffinity(int cpuId);

/// Get the CPU affinity of the current thread.
/// @return A thread with no set affinity will return -1.
/// Otherwise, the current CPU ID will be returned.
/// @return #VPL_ERR_PERM The calling process has insufficient privileges to make this call.
int VPLThread_GetAffinity(void);

/// Get the maximum valid CPU ID available to the application.
/// CPU IDs from 0 up to and including this function's return value may be used when
/// setting affinity.
/// @return A value of 0 means there is only one CPU. Affinity does not matter.
int VPLThread_GetMaxValidCpuId(void);

/// Set the scheduling priority of the calling thread.
/// @param[in] vplPrio New thread priority
/// @return #VPL_OK if the priority was successfully set.
/// @return #VPL_ERR_INVALID @a vplPrio is out-of-bounds [#VPL_PRIO_MIN..#VPL_PRIO_MAX]
/// @return #VPL_ERR_PERM The calling process has insufficient privileges to make this call
int VPLThread_SetSchedPriority(int vplPrio);

/// Get the scheduling priority of the calling thread.
/// @return Current (positive) priority level.
int VPLThread_GetSchedPriority(void);

//--------------------------------------------------------

/// The handle that can be used to join or detach the thread later.
typedef VPLDetachableThreadHandle__t  VPLDetachableThreadHandle_t;

/// Numeric identifier for the thread that can be used for logging and comparison.
typedef uint64_t VPLThreadId_t;

/// The only valid value for #VPLDetachableThread_return_t.
#define VPLTHREAD_RETURN_VALUE  VPLTHREAD__RETURN_VALUE

/// Platform-specific part of a #VPLDetachableThread_fn_t declaration.
#define VPLTHREAD_FN_DECL  VPLDetachableThread__returnType_t VPLTHREAD__FN_ATTRS

/// Type of the start routine passed to #VPLDetachableThread_Create().
/// Note that VPLDetachableThread does not support returning a meaningful value from a thread;
/// you must return #VPLTHREAD_RETURN_VALUE.
///
/// Example: (static is optional)
/// <code>
///     static VPLTHREAD_FN_DECL myThreadFunc(void* arg)
///     {
///         [do stuff...]
///         return VPLTHREAD_RETURN_VALUE;
///     }
/// </code>
typedef VPLDetachableThread__returnType_t (VPLTHREAD__FN_ATTRS *VPLDetachableThread_fn_t)(void*);

/// <code>printf()</code> format directive to print a #VPLThreadId_t.
#define PRI_VPLThreadId_t  PRIu64
#define FMT_VPLThreadId_t  "%"PRI_VPLThreadId_t

/// Create and start a thread executing @a startRoutine with the given parameters.
/// @note To avoid leaking resources, you MUST do exactly ONE of the following:
///     1. Create the thread detached by setting #VPLThread_AttrSetDetachState() to true for @a attrs.
///     2. Call #VPLDetachableThread_Detach() on @a threadHandle_out.
///     3. Call #VPLDetachableThread_Join() on @a threadHandle_out.
/// @note The thread starts immediately. If the thread should wait for some
///     other condition before doing specific operations, use the #VPLCond_t
///     facilities to wait for the condition in @a startRoutine.
/// @param[out] threadHandle_out On success, this will be set to the handle for the new thread.
///             Can be #NULL only if @a attrs request a detached thread
///             (see #VPLThread_AttrSetDetachState()).
/// @param[in] startRoutine Main routine for the thread; you must use #VPLTHREAD_FN_DECL when
///            declaring this function.  See #VPLDetachableThread_fn_t for details.
/// @param[in] startArg Argument to pass to the thread's entrypoint, @a startRoutine.
/// @param[in] attrs A pointer to an initialized VPLThread_attr_t*, or NULL.
///            If NULL, #VPLDetachableThread_Create() will create a thread with "default" attributes:
///            the new thread inherits the priority of the calling thread,
///            and a thread stack-size set to a VPL-defined default value.
/// @param[in] threadName (Optional) name of the thread, as a null-terminated string.  This can
///            be useful for debugging.  Can be #NULL if you don't want to name the thread.
/// @return #VPL_OK Success; thread started.
/// @return #VPL_ERR_MAX  The calling process has reached its limit of #VPLTHREAD_MAX_THREADS
///            concurrent threads.
/// @return #VPL_ERR_AGAIN Insufficient system resources to create the thread.
/// @return #VPL_ERR_INVALID @a startRoutine was NULL.
/// @return #VPL_ERR_INVALID @a threadHandle_out was NULL but the thread is not being created detached.
/// @return #VPL_ERR_PERM The calling process does not have appropriate system permissions for this call.
int VPLDetachableThread_Create(VPLDetachableThreadHandle_t* threadHandle_out,
                     VPLDetachableThread_fn_t startRoutine,
                     void* startArg,
                     const VPLThread_attr_t* attrs,
                     const char* threadName);

/// Detach the indicated thread so that its underlying OS resources will be automatically
/// cleaned up once the thread exits.
/// A detached thread is no longer joinable (you cannot call #VPLDetachableThread_Join() on it),
/// and this call invalidates @a handle.
/// @return #VPL_OK if @a thread was successfully detached.
/// @return #VPL_ERR_INVALID @a handle is NULL.
/// @return #VPL_ERR_ALREADY The thread indicated by @a handle was already detached or joined.
int VPLDetachableThread_Detach(VPLDetachableThreadHandle_t* handle);

/// Wait for the indicated thread to complete.
/// The indicated thread's underlying OS resources will be cleaned up.
/// You cannot call this more than once for any given thread, and
/// and this call invalidates @a handle.
/// @return #VPL_OK if @a thread was successfully detached.
/// @return #VPL_ERR_INVALID @a handle is NULL.
/// @return #VPL_ERR_ALREADY The thread indicated by @a handle was already detached or joined.
int VPLDetachableThread_Join(VPLDetachableThreadHandle_t* handle);

/// Get the identifier for the calling thread.
/// @return Thread ID of calling thread.
VPLThreadId_t VPLDetachableThread_Self(void);

//--------------------------------------------------------

/// @name Dynamic package initialization
///
/// #VPLThread_Once() provides a way for a library to be dynamically
/// initialized. Each such library must declare a global variable of type
/// #VPLThread_once_t to track the initialization state and an initialization
/// routine. Each entry point to the library must start with a call to
/// #VPLThread_Once() with the #VPLThread_once_t variable and initialization
/// routine. This API guarantees that initialization will happen only once and
/// will be complete before #VPLThread_Once() returns.
//@{

/// "Once" control type. Used with #VPLThread_Once().
typedef VPLThread__once_t VPLThread_once_t;

/// Initializer for #VPLThread_once_t.
///
/// Always initialize #VPLThread_once_t variables before use. Example:
///
/// \code VPLThread_once_t myOnceControl = VPLTHREAD_ONCE_INIT; \endcode
#define VPLTHREAD_ONCE_INIT VPLTHREAD__ONCE_INIT

/// Callback function for #VPLThread_Once.
typedef void(*VPLThread_OnceFn_t)(void);

/// Executes the specified function once.
///
/// The first call to this by any thread in a process, with a given
/// #VPLThread_once_t, will call the \a initRoutine callback.
/// Any other threads that call this with the same @a onceControl will block
/// until the first thread returns from the #VPLThread_OnceFn_t callback.
/// Subsequent calls with the same \a onceControl will have no effect and
/// will immediately return #VPL_OK.
/// \a onceControl must have global scope. Behavior of
/// #VPLThread_Once() is not defined if \a onceControl has automatic
/// storage duration or is not initialized by #VPLTHREAD_ONCE_INIT.
/// @note You are allowed to use this function before calling #VPL_Init().  This makes it possible
///     to use #VPLThread_Once() to ensure that #VPL_Init() is not concurrently called from multiple threads.
/// @param[in, out] onceControl Initialization state.
/// @param[in] initRoutine Initialization routine to call the first time
///        #VPLThread_Once() is called with this \a onceControl.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_INVALID Either \a onceControl or \a initRoutine is
///         invalid.
int VPLThread_Once(VPLThread_once_t* onceControl,
                   VPLThread_OnceFn_t initRoutine);

//% End of VPLThread_Once
//@}

//% end VPL Thread-Synchronization Primitives
///@}
//============================================================================

#ifdef  __cplusplus
}
#endif

#endif // include guard
