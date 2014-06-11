//
//  Copyright (C) 2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_QUEUE_H__
#define __VPLEX_QUEUE_H__

//============================================================================
/// @file
/// Builds a singly-linked list queue on top of #VPLListNode_t. Supports
/// inserting at the end (enqueue) and removing from the front (dequeue) in
/// constant time.
//============================================================================

#include "vplex_list.h"

typedef VPLListNode_t  VPLQueueNode_t;
typedef VPLListNode_matchFn_t VPLQueueNode_matchFn_t;

/// A singly-linked list queue.  Supports inserting at the end and
/// removing from the front in constant time.
typedef struct VPLQueue
{
    VPLQueueNode_t* first;
    VPLQueueNode_t* last;
} VPLQueue_t;

static inline NO_NULL_ARGS void VPLQueue_Init(VPLQueue_t* queue)
{
    queue->first = NULL;
    queue->last = NULL;
}

// If either first or last is NULL, both should be.
// (would prefer this to be an inline function, but then we lose the caller's location)
#define VPLQUEUE_ASSERT_INVARIANTS(queue) \
    BEGIN_MULTI_STATEMENT_MACRO \
    ASSERT_NOT_NULL(queue); \
    if(queue->first == NULL) { \
        ASSERTMSG(queue->last == NULLPTR, "first=NULL, last="FMT0xPTR, queue->last); \
    } \
    else if(queue->last == NULL) { \
        ASSERTMSG(queue->first == NULLPTR, "first="FMT0xPTR", last=NULL", queue->first); \
    } \
    END_MULTI_STATEMENT_MACRO

void VPLQueue_Enqueue(VPLQueue_t* queue, VPLQueueNode_t* node) NO_NULL_ARGS;

VPLQueueNode_t* VPLQueue_Dequeue(VPLQueue_t* queue) NO_NULL_ARGS;

static inline NO_NULL_ARGS VPLQueueNode_t* VPLQueue_Peek(VPLQueue_t* queue)
{
    return queue->first;
}

/**
 * @return A linked list of removed nodes.
 */
VPLQueueNode_t* VPLQueue_FindAndRemove(VPLQueue_t* queue,
                                      VPLQueueNode_matchFn_t match_func,
                                      const void* criteria,
                                      VPL_BOOL removeAll);

static inline NO_NULL_ARGS bool VPLQueue_IsEmpty(VPLQueue_t* queue)
{
    return (queue->first == NULL);
}


#endif // include guard
