//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_TH_H__
#define __VPL_TH_H__

//============================================================================
/// @file
/// Virtual Platform Layer API for creating, using, and synchronizing threads.
/// @see VPLThreads
/// @see VPLThreadSynchronization
//============================================================================

#include "vpl_plat.h"
#include "vpl_time.h"

#include "vpl_thread.h"

#include <stdlib.h>  // for size_t
#include <stddef.h> // for ptrdiff_t


#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLThreadSynchronization VPL Thread Synchronization API
///
/// VPL functionality for synchronizing threads.
///
/// VPL provides the following synchronization primitives:
/// - @ref VPLMutex
/// - @ref VPLSemaphore
/// - @ref VPLConditionVariable
/// - @ref VPLRWLocks
/// .
///@{

//-----------------------------------------------
/// @name Thread synchronization debugging aids
/// Use these debugging aids to allow VPL thread synchronization operations
/// to detect use of uninitialized or undefined synchronization objects.
/// Note: VPL_IS_UNDEF works best if the corresponding \<type>_SET_UNDEF macro
///       was used at object declaration.
///@{

//% Being in the same @name group gives all three of these the same description:
/// Test to detect the current state of a thread synchronization object
/// within the program.
/// @hideinitializer
#define VPL_IS_INITIALIZED(obj)   VPL__IS_INITIALIZED(obj)  
#define VPL_IS_UNDEF(obj)         VPL__IS_UNDEF(obj)        
#define VPL_IS_UNINITIALIZED(obj) VPL__IS_UNINITIALIZED(obj)
//

/// Set an object to the uninitialized state.
/// Use this when it is not practical to use the \<type>_SET_UNDEF macro 
/// at object declaration.
#define VPL_SET_UNINITIALIZED(obj) VPL__SET_UNINITIALIZED(obj)

//% end of VPL-thread-synch-debugging aids
///@}

//-----------------------------------------------
/// @defgroup VPLSemaphore VPL Counting Semaphore
/// Implementation of a flexible counting semaphore.
/// A counting semaphore's value can be any nonnegative integer, in contrast
/// to a binary semaphore (which is restricted to 0 and 1).
/// When initializing a semaphore via #VPLSem_Init(), you can specify the maximum
/// value.  By specifying a maximum value of 1, you can obtain a binary semaphore.
///@{

/// Semaphore object type.
typedef VPLSem__t  VPLSem_t;

/// Use this macro to set a just-declared semaphore to the undefined state.
/// Example:
/// \code VPLSem_t sample_sem = VPLSEM_SET_UNDEF; \endcode
/// You can use #VPL_IS_INITIALIZED to check the state.
/// @hideinitializer
#define VPLSEM_SET_UNDEF  VPLSEM__SET_UNDEF

//% This is to enable us to use the native Win32 semaphores, since they use a signed long.
/// Maximum allowed count for a #VPLSem_t.  See #VPLSem_Init().
#define VPLSEM_MAX_COUNT  0x7FFFFFFF

/// Initialize a semaphore object.
/// @param[in,out] sem Pointer to semaphore object.  Must not be NULL.
/// @param[in] maxCount Maximum value the semaphore may attain via post operations.
///     This cannot be larger than #VPLSEM_MAX_COUNT.
/// @param[in] value Initial value of the semaphore, no more than @a maxCount.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_INVALID @a sem is NULL,
///                          or @a maxCount < @a value
/// @return #VPL_ERR_IS_INIT @a sem refers to an already initialized semaphore.
/// @return #VPL_ERR_NOBUFS Insufficient system resources to execute this call.
/// @return #VPL_ERR_MAX The calling process has reached its limit of #VPLTHREAD_MAX_SEMAPHORES.
int VPLSem_Init(VPLSem_t* sem,
                unsigned int maxCount,
                unsigned int value);

/// Wait forever for the semaphore to become ready.
/// When #VPLSem_Wait() returns successfully, the count on the semaphore is decremented by one.
/// As threads call #VPLSem_Post(), the count is incremented and threads blocking
/// on #VPLSem_Wait() or #VPLSem_TimedWait are unblocked.
/// @param[in,out] sem Pointer to semaphore object.
/// @return #VPL_OK @a sem was ready.
/// @return #VPL_ERR_FAIL System-level failure, likely due to a deadlock.
/// @return #VPL_ERR_INVALID @a sem is NULL,
///                          or @a sem does not refer to a valid, initialized semaphore.
int VPLSem_Wait(VPLSem_t* sem);

/// Wait for the semaphore to become ready.
/// When #VPLSem_TimedWait() returns successfully, the count on the semaphore is decremented by one.
/// As threads call #VPLSem_Post(), the count is incremented and threads blocking
/// on #VPLSem_Wait() or #VPLSem_TimedWait are unblocked.
/// @param[in,out] sem Pointer to semaphore object.
/// @return #VPL_OK @a sem was ready.
/// @return #VPL_ERR_TIMEOUT The @a timeout was reached.
/// @return #VPL_ERR_FAIL System-level failure, likely due to a deadlock.
/// @return #VPL_ERR_INVALID @a sem is NULL,
///                          or @a sem does not refer to a valid, initialized semaphore.
int VPLSem_TimedWait(VPLSem_t* sem, VPLTime_t timeout);

/// Check the semaphore to see if it is immediately ready.
/// Does not block on the state of the semaphore.
/// @param[in,out] sem Pointer to semaphore object.
/// @return #VPL_OK @a sem was ready and its count is decremented.
/// @return #VPL_ERR_AGAIN @a sem is busy and acquiring a lock would have blocked.
/// @return #VPL_ERR_FAIL System-level failure, likely due to a deadlock;
///         @a sem is probably rendered invalid.
/// @return #VPL_ERR_INVALID @a sem is NULL,
///                          or @a sem does not refer to a valid, initialized semaphore.
int VPLSem_TryWait(VPLSem_t* sem);

/// Post the semaphore, making it ready.
/// When VPLSem_Post returns the count on the semaphore is incremented.
/// @param[in,out] sem Pointer to semaphore object.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_FAIL System-level failure, likely due to a deadlock.
/// @return #VPL_ERR_INVALID @a sem is NULL,
///                          or @a sem does not refer to a valid, initialized semaphore.
/// @return #VPL_ERR_MAX @a sem has already reached its max count.
int VPLSem_Post(VPLSem_t* sem);

/// Destroy a semaphore.
/// Behavior of threads waiting for the semaphore is undefined.
/// @param[in] sem Pointer to semaphore object.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_FAIL System-level failure, deallocation of resources failed.
/// @return #VPL_ERR_INVALID @a sem is NULL,
///                          or @a sem does not refer to a valid, initialized semaphore.
int VPLSem_Destroy(VPLSem_t* sem);

//% end VPL counting semaphore
///@}

//-----------------------------------------------
/// @defgroup VPLMutex VPL Mutex
/// Implementation of a standard mutex.
///@{

/// Mutex object type.
typedef VPLMutex__t  VPLMutex_t;

/// Use this macro to set a just-declared mutex to the undefined state.
/// Example:
/// \code VPLMutex_t sample_mutex = VPLMUTEX_SET_UNDEF; \endcode
/// You can use #VPL_IS_INITIALIZED to check the state.
/// @hideinitializer
#define VPLMUTEX_SET_UNDEF  VPLMUTEX__SET_UNDEF

/// Initialize a mutex object.
/// @param[in,out] mutex Pointer to a mutex object. Must not be NULL.
/// @return #VPL_OK if successful
/// @return #VPL_ERR_AGAIN insufficient system resources to initialize another mutex
/// @return #VPL_ERR_IS_INIT @a mutex refers to an already initialized mutex
/// @return #VPL_ERR_NOMEM insufficient memory to initialize another mutex
/// @return #VPL_ERR_INVALID @a mutex is NULL
/// @return #VPL_ERR_PERM the calling process has insufficient privileges for this call
/// @return #VPL_ERR_MAX the calling process has reached its limit of #VPLTHREAD_MAX_MUTEXES
int VPLMutex_Init(VPLMutex_t* mutex);

/// Lock a mutex.
/// Block on a mutex until it can be locked. If the mutex has already been
/// locked by the calling thread, the lock-count on the mutex will be incremented
/// and the function will return successfully.
/// @param[in,out] mutex Pointer to a mutex object.
/// @return #VPL_OK if @a mutex is locked
/// @return #VPL_ERR_AGAIN @a mutex could not be acquired because the maximum number
///         of recursive locks has been exceeded
/// @return #VPL_ERR_INVALID @a mutex is NULL, or does not refer to a valid, initialized mutex
int VPLMutex_Lock(VPLMutex_t* mutex);

/// Attempt to lock a mutex.
/// Non-blockingly attempt to lock a mutex.  If the mutex is in a state where
/// it can be locked it will be locked.  Otherwise, it will not be locked.  Result
/// is indicated by return value.
/// @param[in,out] mutex Pointer to a mutex object.
/// @return #VPL_OK Success; @a mutex was locked.
/// @return #VPL_ERR_BUSY The mutex is in a state where it cannot be locked.
/// @return #VPL_ERR_INVALID @a mutex is NULL, or does not refer to a valid, initialized mutex.
int VPLMutex_TryLock(VPLMutex_t* mutex);

/// Unlock a mutex.
/// Will unlock a mutex that has been locked by the calling thread.
/// #VPLMutex_Unlock() checks to make sure that the mutex is both locked
/// and has been locked by the calling thread.  An error will be returned otherwise.
/// Unlocking will result in the mutex lock count being decremented. If the count
/// is == 0 then the mutex will become unlocked and available to be locked by other
/// threads.
/// @param[in,out] mutex Pointer to a mutex object.
/// @return #VPL_OK Success; the lock count was decremented.
/// @return #VPL_ERR_AGAIN @a mutex could not be acquired because the maximum number
///         of recursive locks has been exceeded.
/// @return #VPL_ERR_INVALID @a mutex is NULL, or does not refer to a valid, initialized mutex.
/// @return #VPL_ERR_PERM The mutex is not locked by this thread.
int VPLMutex_Unlock(VPLMutex_t* mutex);

/// Destroy a mutex.
/// @param[in] mutex Pointer to a mutex object.
/// @return #VPL_OK Success; @a mutex was destroyed.
/// @return #VPL_ERR_BUSY @a mutex is still locked or referenced. This check is advisory only;
///     program correctness should not rely upon this behavior, as this condition may not always
///     be detected.
/// @return #VPL_ERR_INVALID @a mutex is NULL, or does not refer to a valid, initialized mutex.
/// @note The mutex must not be locked when destroyed.
int VPLMutex_Destroy(VPLMutex_t* mutex);

/// Test if the mutex is locked by any thread.
/// @param[in] mutex Pointer to a mutex object.
/// @return #VPL_ERR_INVALID @a mutex is NULL.
/// @return non-zero if locked, zero if not locked.
int VPLMutex_Locked(const VPLMutex_t* mutex);

/// Test if the mutex is locked by this thread.
/// @param[in] mutex Pointer to a mutex object.
/// @return #VPL_ERR_INVALID @a mutex is NULL.
/// @return non-zero if locked by this thread, zero otherwise.
int VPLMutex_LockedSelf(const VPLMutex_t* mutex);

//% end VPL mutex
///@}

//-----------------------------------------------
/// @defgroup VPLConditionVariable VPL Condition Variable
/// Implementation of a condition variable.
/// A condition variable is typically used to implement the monitor pattern.
/// Please see pthread documentation and tutorials for more information.
///@{

/// Condition variable object type.
typedef VPLCond__t  VPLCond_t;

/// Use this macro to set a just-declared condition variable to the undefined state.
/// Example:
/// \code VPLCond_t sample_condvar = VPLCOND_SET_UNDEF; \endcode
/// You can use #VPL_IS_INITIALIZED to check the state.
/// @hideinitializer
#define VPLCOND_SET_UNDEF  VPLCOND__SET_UNDEF

/// Initialize a condition variable object.
/// @param[in,out] cond Pointer to a cond variable object. Must not be NULL.
/// @return #VPL_OK if @a cond is initialized
/// @return #VPL_ERR_AGAIN the system has insufficient resources to init
///                 another condition variable
/// @return #VPL_ERR_IS_INIT @a cond refers to an already initialized condition
///                 variable
/// @return #VPL_ERR_INVALID @a cond is NULL
/// @return #VPL_ERR_NOMEM the system has insufficient memory to init another
///                 condition variable
/// @return #VPL_ERR_MAX the calling process has reached its limit of
///                 #VPLTHREAD_MAX_CONDVARS concurrent #VPLCond_t objects
int VPLCond_Init(VPLCond_t* cond);

/// Wait for a condition variable to become true, or until timeout.
/// #VPLCond_TimedWait() will block on a condition variable, @a cond, until
/// it becomes available. @a mutex must be locked by the calling thread
/// or undefined behavior will result. The same @a mutex must be supplied to
/// concurrent #VPLCond_TimedWait() operations on the same @a cond, or undefined
/// behavior will result.
/// Upon successful return, @a mutex will be locked and owned by the calling thread.
/// Like pthread Condition Variables, a #VPLCond_t should be used with an
/// associated boolean predicate.  Because spurious wake ups from
/// #VPLCond_TimedWait() can occur, the value of the boolean predicate should
/// always be re-evaluated upon return.
/// @param[in,out] cond Pointer to a cond variable object.
/// @param[in,out] mutex Pointer to a (locked) mutex object.
///              The mutex is unlocked while this thread waits,
///              and is locked again when this call returns.
/// @param[in] time Timeout for the wait, in microseconds.
///             Use #VPL_TIMEOUT_NONE to wait forever.
/// @return #VPL_OK if returning because #VPLCond_Signal() was called
/// @return #VPL_ERR_TIMEOUT if returning due to timeout
/// @return #VPL_ERR_INVALID @a cond or @a mutex is NULL or uninitialized
int VPLCond_TimedWait(VPLCond_t* cond, 
                      VPLMutex_t* mutex, 
                      VPLTime_t time);

/// Signal a condition variable, waking (at least) one waiting thread.
/// #VPLCond_Signal() is used to unblock threads that are blocked on a
/// @a cond.  It will unblock at least one of those other threads
/// that are blocked on @a cond.  The "at least" provision
/// is why external boolean predicates must be used along side VPL condition
/// variables.  This function has no effect if there are no threads currently
/// blocking on @a cond, so a thread that calls this function should usually
/// have the associated mutex locked first.
/// @param[in,out] cond Pointer to a cond variable object.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_INVALID @a cond is NULL, or does not refer to a valid,
///         initialized condition variable.
int VPLCond_Signal(VPLCond_t* cond);

/// Broadcast a condition variable, waking all waiting threads.
/// #VPLCond_Broadcast() is used to unblock threads that are blocked on a
/// @a cond.  It will unblock all of those other threads that are blocked
/// on @a cond. This function has no effect if there are no threads currently
/// blocking on @a cond, so a thread that calls this function should usually
/// have the associated mutex locked first.
/// @param[in,out] cond Pointer to a cond variable object.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_INVALID @a cond is NULL, or does not refer to a valid,
///         initialized condition variable.
int VPLCond_Broadcast(VPLCond_t* cond);

/// Destroy a condition variable.
/// @param[in] cond Pointer to a cond variable object.
/// @return #VPL_OK Success; @a cond is destroyed.
/// @return #VPL_ERR_INVALID @a cond is NULL, or does not refer to a valid,
///         initialized condition variable, or @a cond is still referenced
///         by another thread.
/// @note Behavior of waiting threads when the cond variable is destroyed
///       is undefined.
int VPLCond_Destroy(VPLCond_t* cond);

//% end VPL condition variable
///@}


//% end VPL Thread-Synchronization Primitives
///@}
//============================================================================

#ifdef  __cplusplus
}
#endif

#endif // include guard
