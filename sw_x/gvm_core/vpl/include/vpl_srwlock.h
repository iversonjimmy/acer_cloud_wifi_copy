//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef VPL_SRWLOCK_H__
#define VPL_SRWLOCK_H__

//============================================================================
/// @file
/// Virtual Platform Layer API for slim reader-writer locks.
/// @see VPLSlimRWLocks
//============================================================================

#include "vpl__srwlock.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------
/// @defgroup VPLSlimRWLocks VPL Slim Reader-Writer Locks
/// @ingroup VPLThreadSynchronization
/// Reader-writer locks protect a shared resource where any number of readers (shared mode) or
/// one writer (exclusive mode) may concurrently access the resource.
/// VPL Slim Reader-Writer locks cannot be acquired recursively, but they more efficient
/// than regular reader-writer locks.
/// VPL Slim Reader-Write locks are neither fair nor FIFO.
///@{

/// Slim reader-writer lock type.
typedef VPLSlimRWLock__t VPLSlimRWLock_t;

///
/// Initialize a slim reader-writer lock.
///
/// Any resources required for the reader-writer lock will be acquired and the
/// lock will be initialized to an unlocked state. Results are undefined if
/// \a rwlock has already been initialized.
///
/// @param[in,out] rwlock Pointer to a slim reader-writer lock.
/// @return #VPL_OK \a rwlock has been successfully initialized.
/// @return #VPL_ERR_BUSY \a rwlock points to a reader-writer lock that was
///     previously initialized but not yet destroyed. This condition may not
///     always be detected.
/// @return #VPL_ERR_INVALID \a rwlock is NULL.
/// @return #VPL_ERR_NOMEM Insufficient memory to complete initialization.
/// @return #VPL_ERR_PERM Caller lacks permission to initialize a reader-writer lock.
int VPLSlimRWLock_Init(VPLSlimRWLock_t* rwlock);

///
/// Destroy a previously initialized slim reader-writer lock.
///
/// Any resources acquired at #VPLSlimRWLock_Init() are released.
/// Behavior is undefined if the reader-writer lock is locked when calling
/// #VPLSlimRWLock_Destroy().
///
/// @param[in,out] rwlock Pointer to a slim reader-writer lock.
/// @return #VPL_OK \a rwlock has been successfully destroyed.
/// @return #VPL_ERR_BUSY \a rwlock is locked. This condition may not always be detected.
/// @return #VPL_ERR_INVALID \a rwlock is NULL.
int VPLSlimRWLock_Destroy(VPLSlimRWLock_t* rwlock);

///
/// Acquire the lock for reading, blocking until the lock can be acquired.
///
/// The calling thread will acquire the lock in reader (shared) mode at some point when the lock is
/// not being held in writer (exclusive) mode by any thread.
/// Recursive locking is not supported, so do not call this if the current thread already
/// holds the lock (whether for reading or writing).
///
/// @param[in,out] rwlock Pointer to a slim reader-writer lock.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_INVALID \a rwlock is NULL.
/// @return #VPL_ERR_DEADLK The calling thread already holds the write lock for @a rwlock.
///     This condition may not always be detected, resulting in deadlock.
int VPLSlimRWLock_LockRead(VPLSlimRWLock_t* rwlock);

///
/// Acquire the lock for writing, blocking until the lock can be acquired.
///
/// The calling thread will acquire the lock in writer (exclusive) mode at some point when the lock
/// is not being held by any thread.
/// Recursive locking is not supported, so do not call this if the current thread already
/// holds the lock (whether for reading or writing).
///
/// @param[in,out] rwlock Pointer to a slim reader-writer lock.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_INVALID \a rwlock is NULL.
/// @return #VPL_ERR_DEADLK The calling thread already holds a read or write lock
///    for @a rwlock. This condition may not always be detected, resulting in deadlock.
int VPLSlimRWLock_LockWrite(VPLSlimRWLock_t* rwlock);

///
/// Release a lock held in reader (shared) mode.
///
/// Results are undefined if the calling thread does not hold the lock in reader mode.
///
/// @param[in,out] rwlock Pointer to a slim reader-writer lock.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_INVALID \a rwlock is NULL.
/// @return #VPL_ERR_PERM The current thread does not hold a lock on @a rwlock.
///     This error may not be reported.
int VPLSlimRWLock_UnlockRead(VPLSlimRWLock_t* rwlock);

///
/// Release a lock held in writer (exclusive) mode.
///
/// Results are undefined if the calling thread does not hold the lock in writer mode.
///
/// @param[in,out] rwlock Pointer to a slim reader-writer lock.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_INVALID \a rwlock is NULL.
/// @return #VPL_ERR_PERM The current thread does not hold a lock on @a rwlock.
///     This error may not be reported.
int VPLSlimRWLock_UnlockWrite(VPLSlimRWLock_t* rwlock);

//% end of VPL Slim Reader-Writer Locks
///@}

#ifdef __cplusplus
}
#endif

#endif // include guard
