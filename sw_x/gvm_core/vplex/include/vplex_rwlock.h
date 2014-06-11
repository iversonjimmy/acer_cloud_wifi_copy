//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_RWLOCK_H__
#define __VPLEX_RWLOCK_H__

//============================================================================
/// @file
/// Provides a Readers-Writers lock.
//============================================================================

#include "vpl_th.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================
// Public interface
//=============================================

typedef struct VPLLock__t  VPLLock_t;

typedef struct VPLThread_id_node__t  VPLThread_id_node_t;

#define FMT_VPLLock_t  "RWLock("FMT_PTR")"

int VPLLock_Init(VPLLock_t* lock);

int VPLLock_LockWrite(VPLLock_t* lock);

/// @note \a tempStorage is used to track the active readers, so it needs to stay in scope
/// until #VPLLock_Unlock() is called.
int VPLLock_LockRead(VPLLock_t* lock, VPLThread_id_node_t* tempStorage);

/// Returns true if the current thread owns the lock.
bool VPLLock_IsWriteLocked(VPLLock_t const* lock);

/// Returns true if the current thread is an active reader.
bool VPLLock_IsReadLocked(VPLLock_t const* lock);

/// Returns true if the current thread is an active reader or writer.
bool VPLLock_IsLocked(VPLLock_t const* lock);

int VPLLock_Unlock(VPLLock_t* lock);

/// @note This call probably still isn't safe.
int VPLLock_Destroy(VPLLock_t* lock);

//=============================================
// Should treat these as opaque types:
//=============================================
struct VPLThread_id_node__t {
    VPLThread_id_node_t* next;
    VPLThread_t threadId;
};

#ifndef VPL_PLAT_IS_WINRT
typedef struct VPLLock__QueuedReaderNode  VPLLock_QueuedReaderNode;
#endif
typedef struct VPLLock_QueuedReaderNode {
    struct VPLLock_QueuedReaderNode* next;
    VPLThread_id_node_t* threadIdNode;
    VPLSem_t semaphore;
} VPLLock_QueuedReaderNode_t;

typedef struct VPLLock_QueuedWriterNode {
    struct VPLLock_QueuedWriterNode* next;
    VPLThread_t threadId;
    VPLSem_t semaphore;
} VPLLock_QueuedWriterNode_t;

struct VPLLock__t {
    VPLMutex_t lockMutex;
    bool isActiveWriter;
    VPLThread_t writer;
    VPLLock_QueuedWriterNode_t* firstQueuedWriter;
    VPLLock_QueuedWriterNode_t* lastQueuedWriter;
    VPLThread_id_node_t* activeReaders;
    VPLLock_QueuedReaderNode_t* queuedReaders;
};
//=============================================

#ifdef  __cplusplus
}
#endif

#endif // include guard
