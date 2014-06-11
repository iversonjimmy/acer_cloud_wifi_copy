//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_assert.h"
#include "vpl_types.h"
#include "vplex_private.h"

#include "vplex_rwlock.h"

//============================================================================
/// @file
/// Platform independent implementation of a Readers-Writers lock.
//============================================================================

int VPLLock_Init(VPLLock_t* lock)
{
    int rv;
    VPL_LIB_LOG_FINER(VPL_SG_TH, "Init lock: "FMT_VPLLock_t" by "FMT_VPLThread_t,
                lock, VAL_VPLThread_t(VPLThread_Self()));
    lock->isActiveWriter = false;
    // writer is only valid when isActiveWriter is true.
    lock->firstQueuedWriter = NULL;
    lock->lastQueuedWriter = NULL;
    lock->activeReaders = NULL;
    lock->queuedReaders = NULL;
    rv = VPLMutex_Init(&lock->lockMutex);
    ASSERT_VPL_OK(rv);
    return rv;
}

/// This might legitimately fail if a thread is waiting for the lock when the lock
/// is destroyed by another thread.
#define LOCK_OR_RETURN(rv, lock) \
    BEGIN_MULTI_STATEMENT_MACRO \
    rv = VPLMutex_Lock(&(lock)->lockMutex); \
    if (rv != VPL_OK) {\
        VPL_LIB_LOG_WARN(VPL_SG_TH, "VPLMutex_Lock failed:%d ", rv);    \
        return rv; \
    } \
    END_MULTI_STATEMENT_MACRO
#define LOCK_OR_RETURN_FALSE(rv, lock) \
    BEGIN_MULTI_STATEMENT_MACRO \
    rv = VPLMutex_Lock(&(lock)->lockMutex); \
    if (rv != VPL_OK) {\
        VPL_LIB_LOG_WARN(VPL_SG_TH, "VPLMutex_Lock failed:%d", rv); \
        return false; \
    } \
    END_MULTI_STATEMENT_MACRO

#define UNLOCK_LOCK(rv, lock) \
    rv = VPLMutex_Unlock(&(lock)->lockMutex); \
    ASSERT_VPL_OK(rv);

static bool _isCurrThreadActiveWriter(VPLLock_t const* lock)
{
    VPLThread_t thisThread = VPLThread_Self();
    return (lock->isActiveWriter) && VPLThread_Equal(&lock->writer, &thisThread);
}

static bool _isCurrThreadActiveReader(VPLThread_id_node_t const* listNode)
{
    VPLThread_t thisThread = VPLThread_Self();
    for(;
        listNode != NULL;
        listNode = listNode->next)
    {
        if(VPLThread_Equal(&listNode->threadId, &thisThread))
            return true;
    }
    return false;
}

int VPLLock_LockWrite(VPLLock_t* lock)
{
    int rv;
    VPL_LIB_LOG_FINEST(VPL_SG_TH, "Lock for write: "FMT_VPLLock_t" by "FMT_VPLThread_t,
                lock, VAL_VPLThread_t(VPLThread_Self()));
    LOCK_OR_RETURN(rv, lock);
    if (_isCurrThreadActiveReader(lock->activeReaders)) {
        FAILED_ASSERT("Already a reader!");
    }
    else if(_isCurrThreadActiveWriter(lock)) {
        FAILED_ASSERT("Already a writer!");
    }
    else {
        if( (lock->isActiveWriter) ||
            (lock->activeReaders != NULL) )
        {
            /// Add to the queued writers.
            VPLLock_QueuedWriterNode_t writerNode;
            // Explicitly set this to avoid accessing truly uninitialized memory.
            VPL_SET_UNINITIALIZED(&writerNode.semaphore);
            writerNode.threadId = VPLThread_Self();
            rv = VPLSem_Init(&writerNode.semaphore, 1, 0);
            ASSERT_VPL_OK(rv);
            if(lock->lastQueuedWriter != NULL)
            {
                lock->lastQueuedWriter->next = &writerNode;
            }
            else
            {
                /// The queue was empty.
                lock->firstQueuedWriter = &writerNode;
            }
            writerNode.next = NULL;
            lock->firstQueuedWriter = &writerNode;
            UNLOCK_LOCK(rv, lock);

            /// Must wait while *unlocked*
            rv = VPLSem_Wait(&writerNode.semaphore);
            ASSERT_VPL_OK(rv);
            rv = VPLSem_Destroy(&writerNode.semaphore);
            ASSERT_VPL_OK(rv);

            LOCK_OR_RETURN(rv, lock);
            ASSERT(_isCurrThreadActiveWriter(lock));
        }
        else
        {
            /// Become the active writer.
            ASSERT(lock->firstQueuedWriter == NULL);
            ASSERT(lock->queuedReaders == NULL);
            lock->isActiveWriter = true;
            lock->writer = VPLThread_Self();
        }
        ASSERT(lock->activeReaders == NULL);
    }
    UNLOCK_LOCK(rv, lock);
    return rv;
}

int VPLLock_LockRead(VPLLock_t* lock, VPLThread_id_node_t* idNode)
{
    int rv;
    VPL_LIB_LOG_FINEST(VPL_SG_TH,
               "Lock for read: "FMT_VPLLock_t" by "FMT_VPLThread_t,
               lock, VAL_VPLThread_t(VPLThread_Self()));
    LOCK_OR_RETURN(rv, lock);
    if(_isCurrThreadActiveReader(lock->activeReaders))
    {
        FAILED_ASSERT("Already a reader!");
    }
    else if(_isCurrThreadActiveWriter(lock))
    {
        FAILED_ASSERT("Already a writer!");
    }
    else
    {
        /// If there is currently an active writer, we must wait.
        /// If there are currently active readers, we can join them only if
        /// there are no queued writers (this is to guarantee that starvation will not occur).
        if( (lock->isActiveWriter) || (lock->firstQueuedWriter != NULL) )
        {
            /// Add to the waiting readers.
            VPLLock_QueuedReaderNode_t readerNode;
            readerNode.threadIdNode = idNode;
            readerNode.threadIdNode->threadId = VPLThread_Self();
            rv = VPLSem_Init(&readerNode.semaphore, 1, 0);
            readerNode.next = lock->queuedReaders;
            lock->queuedReaders = &readerNode;
            UNLOCK_LOCK(rv, lock);

            /// Must wait while *unlocked*
            rv = VPLSem_Wait(&readerNode.semaphore);
            ASSERT_VPL_OK(rv);
            rv = VPLSem_Destroy(&readerNode.semaphore);
            ASSERT_VPL_OK(rv);

            LOCK_OR_RETURN(rv, lock);
            ASSERT(_isCurrThreadActiveReader(lock->activeReaders));
        }
        else
        {
            /// Become an active reader.
            idNode->threadId = VPLThread_Self();
            idNode->next = lock->activeReaders;
            lock->activeReaders = idNode;
        }
        ASSERT(!lock->isActiveWriter);
    }
    UNLOCK_LOCK(rv, lock);
    return VPL_OK;
}

/// Since C doesn't have a parallel to C++'s mutable, we have to cast the const away to temporarily
/// lock the mutex.
#define MAKE_MUTABLE(lock)  ((VPLLock_t*)(lock))

bool VPLLock_IsWriteLocked(VPLLock_t const* lock)
{
    int rv;
    bool result;
    LOCK_OR_RETURN_FALSE(rv, MAKE_MUTABLE(lock));
    result = _isCurrThreadActiveWriter(lock);
    UNLOCK_LOCK(rv, MAKE_MUTABLE(lock));
    return result;
}

bool VPLLock_IsReadLocked(VPLLock_t const* lock)
{
    int rv;
    bool result;
    LOCK_OR_RETURN_FALSE(rv, MAKE_MUTABLE(lock));
    result = _isCurrThreadActiveReader(lock->activeReaders);
    UNLOCK_LOCK(rv, MAKE_MUTABLE(lock));
    return result;
}

bool VPLLock_IsLocked(VPLLock_t const* lock)
{
    int rv;
    bool result;
    LOCK_OR_RETURN_FALSE(rv, MAKE_MUTABLE(lock));
    result = _isCurrThreadActiveWriter(lock) || _isCurrThreadActiveReader(lock->activeReaders);
    UNLOCK_LOCK(rv, MAKE_MUTABLE(lock));
    return result;
}

static void _wakeReaders(VPLLock_t* lock)
{
    /// Dequeue each queued reader.
    for(;
        lock->queuedReaders != NULL;
        lock->queuedReaders = lock->queuedReaders->next)
    {
        int rv;
        VPLLock_QueuedReaderNode_t* const curr = lock->queuedReaders;
        VPLThread_id_node_t* const currThreadIdNode = curr->threadIdNode;
        /// Add it to the active queue.
        currThreadIdNode->next = lock->activeReaders;
        lock->activeReaders = currThreadIdNode;
        /// Signal the semaphore.
        rv = VPLSem_Post(&curr->semaphore);
        ASSERT_VPL_OK(rv);
    }
}

static void _wakeWriter(VPLLock_t* lock)
{
    int rv;
    VPLLock_QueuedWriterNode_t* const curr = lock->firstQueuedWriter;
    ASSERT_NOT_NULL(curr);
    /// Dequeue the next writer.
    lock->firstQueuedWriter = curr->next;
    /// Make it active
    lock->writer = curr->threadId;
    lock->isActiveWriter = true;
    /// Signal the semaphore.
    rv = VPLSem_Post(&curr->semaphore);
    ASSERT_VPL_OK(rv);
}

static void _removeSelfFromActiveReaderList(VPLLock_t* lock)
{
    VPLThread_t thisThread = VPLThread_Self();
    VPLThread_id_node_t* curr = lock->activeReaders;
    VPLThread_id_node_t* prev = NULL;
    for(;
        curr != NULL;
        prev = curr, curr = curr->next)
    {
        if(VPLThread_Equal(&curr->threadId, &thisThread))
        {
            if(prev != NULL)
            {
                prev->next = curr->next;
            }
            else
            {
                /// It was at the front.
                lock->activeReaders = curr->next;
            }
            return;
        }
    }
    ASSERT(false);
}

int VPLLock_Unlock(VPLLock_t* lock)
{
    int rv;
    VPL_LIB_LOG_FINEST(VPL_SG_TH, "Unlock: "FMT_VPLLock_t" by "FMT_VPLThread_t,
               lock, VAL_VPLThread_t(VPLThread_Self()));
    LOCK_OR_RETURN(rv, lock);
    if(lock->isActiveWriter)
    {
        ASSERT(_isCurrThreadActiveWriter(lock));
        ASSERT(lock->activeReaders == NULL);
        lock->isActiveWriter = false;

        /// The lock is now free.
        if(lock->queuedReaders != NULL)
        {
            /// Wake up all of the readers now.
            _wakeReaders(lock);
        }
        else if(lock->firstQueuedWriter != NULL)
        {
            /// No readers are waiting; we can allow the next writer to get the lock.
            _wakeWriter(lock);
        }
        else
        {
            /// Nothing else to do; nobody is waiting.
        }
    }
    else if(lock->activeReaders != NULL)
    {
        ASSERT(_isCurrThreadActiveReader(lock->activeReaders));
        /// Remove the reader from the active readers.
        _removeSelfFromActiveReaderList(lock);
        if(lock->activeReaders == NULL)
        {
            /// This was the last reader.
            if(lock->firstQueuedWriter != NULL)
            {
                /// It's the next writer's turn.
                _wakeWriter(lock);
            }
            else if(lock->queuedReaders != NULL)
            {
                /// This case shouldn't actually happen.
                FAILED_ASSERT("There was no writer waiting, but there were readers waiting!");
                /// Wake up the queued readers.
                _wakeReaders(lock);
            }
            else
            {
                /// Nothing else to do; nobody is waiting.
            }
        }
        else
        {
            /// Nothing else to do; there are other active readers.
        }
    }
    else
    {
        FAILED_ASSERT("Nothing was locked; should not have called unlock.");
    }
    UNLOCK_LOCK(rv, lock);
    return rv;
}

int VPLLock_Destroy(VPLLock_t* lock)
{
    int rv;
    LOCK_OR_RETURN(rv, lock);
    VPL_LIB_LOG_FINER(VPL_SG_TH, "Destroy lock: "FMT_VPLLock_t" by "FMT_VPLThread_t,
                lock, VAL_VPLThread_t(VPLThread_Self()));
    {
        /// First wake up everyone (they should get a failure when they try to re-lock).
        _wakeReaders(lock);
        while (lock->firstQueuedWriter != NULL) {
            _wakeWriter(lock);
        }
        rv = VPLMutex_Destroy(&lock->lockMutex);
        ASSERT_VPL_OK(rv);
    }
    return rv;
}
