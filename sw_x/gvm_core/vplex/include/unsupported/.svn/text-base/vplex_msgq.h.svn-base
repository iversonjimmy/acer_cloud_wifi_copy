//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_MSGQ_H__
#define __VPLEX_MSGQ_H__

//============================================================================
/// @file
/// VPL Thread Synchronization Primitive API: Message Queue
// No longer supported; this should be ported to use VPL mutex and semaphore if it is revived.
//============================================================================

#include "vplex__msgq.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @addtogroup VPLThreadSynchronization
///@{

//-----------------------------------------------
/// @name VPL Message Queue
///@{

/// Maximum concurrent number of VPLMsgQ_t objects.
#define VPLTHREAD_MAX_MSGQUEUES  VPLTHREAD__MAX_MSGQUEUES

/// Message queue object type.
/// A VPL message queue provides a way to pass ordered messages between
/// threads of a given program.
/// The message sender should either send constant messages or pointers to
/// allocated messages. Copies of the messages are not made; only references
/// to the messages (pointers to their buffers) are stored while enqueued.
/// It is up to the user of message queues to ensure memory is properly used
/// for message queue messages.
typedef VPLMsgQ__t VPLMsgQ_t;

/// Use this macro to set a just-declared message queue to the undefined state.
/// Example:
/// \code VPLMsgQ_t sample_msgqueue = VPLMSGQ_SET_UNDEF; \endcode
/// You can use #VPL_IS_INITIALIZED to check the state.
#define VPLMSGQ_SET_UNDEF  VPLMSGQ__SET_UNDEF

/// Initialize a message queue object.
/// An amount of memory equal to sizeof(void*) * \a max_msgs will be allocated
/// for the message queue.
/// @param[in] mq Pointer to the message queue to initialize
/// @param[in] max_msgs Maximum messages in the queue at any time.
/// @return #VPL_OK if the message queue object is initialized
/// @return #VPL_ERR_AGAIN Insufficient system resources to initialize another mutex
/// @return #VPL_ERR_INVALID @a mq is either NULL or has already been initialized
/// @return #VPL_ERR_NOMEM Insufficient memory is available to construct the queue
/// @return #VPL_ERR_PERM Process has insufficient privileges to make this call.
/// @note Memory of at least max_msgs * sizeof(void *) is allocated on success.
/// @return #VPL_ERR_MAX  The calling process has reached its limit of #VPLTHREAD_MAX_MSGQUEUES
///            concurrent message queue objects.
int VPLMsgQ_Init(VPLMsgQ_t* mq, unsigned int max_msgs);

/// Destroy a message queue.
/// Upon destruction the initial memory allocation (sizeof(void*)*max_msgs)
/// will be free'd to the system.  Because the message queue operates
/// simply by passing around references to the actual memory containing
/// the messages themselves, any memory occupied by any of the messages
/// themselves will continue to be the calling application's responsibility
/// @param[in] mq The message queue to destroy.
/// @return #VPL_OK if the message queue object is destroyed,
/// @return #VPL_ERR_INVALID @a mq is either NULL or does not refer to an initialized Message Queue
int VPLMsgQ_Destroy(VPLMsgQ_t* mq);

/// Attempt to atomically enqueue message to the message queue.
/// #VPLMsgQ_Put() will block until the \a msg can be enqueued.
/// @param[in,out] mq The message queue to use.
/// @param[in] msg Pointer to message buffer.
/// @return #VPL_OK if the message is queued,
/// @return #VPL_ERR_AGAIN The message queue is leaking capacity
/// @return #VPL_ERR_DEADLK A deadlock was detected
/// @return #VPL_ERR_INTR A system signal interrupted the call
/// @return #VPL_ERR_INVALID @a mq is NULL or refers to an uninitialized Message Queue or
///                          the internals of @a mq have become corrupted 
int VPLMsgQ_Put(VPLMsgQ_t* mq, void* msg);

/// Attempt to put a message at the end of the queue, return immediately if queue is
/// full.
/// @param[in,out] mq The message queue to use.
/// @param[in] msg Pointer to message buffer.
/// @return #VPL_OK if the message is queued
/// @return #VPL_ERR_AGAIN if the queue is full (try again later)
/// @return #VPL_ERR_INTR A system signal interrupted the call
/// @return #VPL_ERR_INVALID @a mq is NULL or refers to an uninitialized Message Queue
int VPLMsgQ_PutNoWait(VPLMsgQ_t* mq, void* msg);

/// Get the message at the front of the queue. Will block until a \a msg is present
/// @param[in,out] mq The message queue to use.
/// @param[in,out] msg Pointer to buffer for the message.
/// @return #VPL_OK if a message was taken from the queue,
/// @return #VPL_ERR_INVALID @a mq is NULL or refers to an uninitialized Message Queue
int VPLMsgQ_Get(VPLMsgQ_t* mq, void** msg);

/// Get the message at the front of the queue, returning immediately if no
/// messages are pending.
/// @param mq The message queue to use.
/// @param msg Pointer to buffer for the message.
/// @return #VPL_OK if the message was taken from the queue,
/// @return #VPL_ERR_AGAIN if the queue was empty (try again later),
/// @return #VPL_ERR_INVALID @a mq is NULL or refers to an uninitialized Message Queue
/// @note If msg is not NULL, the buffer must be large enough for the maximum
///       message size the queue could have. If it is NULL, the first message
///       in the queue will be discarded on success.
int VPLMsgQ_GetNoWait(VPLMsgQ_t* mq, void** msg);

/// Check if the message queue is empty (no messages to get).
/// @param[in] mq The message queue to use.
/// @return 1 if the message queue is empty when checked.
/// @return 0 if there is at least one message when checked.
/// @return #VPL_ERR_INVALID @a mq is NULL or refers to an uninitialized Message Queue
int VPLMsgQ_IsEmpty(const VPLMsgQ_t* mq);

/// Check if the message queue is full (no space for more messages).
/// @param[in] mq The message queue to use.
/// @return 1 if the message queue is full when checked.
/// @return 0 if the message queue is not full when checked.
/// @return #VPL_ERR_INVALID @a mq is NULL or refers to an uninitialized Message Queue
int VPLMsgQ_IsFull(const VPLMsgQ_t* mq);

/// Get the number of pending messages.
/// @param[in] mq The message queue to use.
/// @return Non-negative number of messages in the queue on success.
/// @return #VPL_ERR_INVALID @a mq is NULL or refers to an uninitialized Message Queue
int VPLMsgQ_GetNumMsgs(const VPLMsgQ_t* mq);

//% end VPL message queue
///@}
//-----------------------------------------------

//% end VPL Thread-Synchronization Primitives
///@}
//============================================================================

#ifdef  __cplusplus
}
#endif

#endif // include guard
