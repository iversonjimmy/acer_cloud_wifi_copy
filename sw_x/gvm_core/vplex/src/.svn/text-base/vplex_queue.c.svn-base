//
//  Copyright (C) 2008-2009, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_queue.h"

/// Set the variable to a marker value, to aid debugging. Can be turned off for release.
#define INVALID_PTR  SIZE_T_TO_PTR(133)
#define MARK_INVALID_PTR(variable)  (variable) = (INVALID_PTR)

void VPLQueue_Enqueue(VPLQueue_t* queue, VPLQueueNode_t* node)
{
    ASSERT_NOT_NULL(node);
    VPLQUEUE_ASSERT_INVARIANTS(queue);
    node->next = NULL;
    if (queue->last == NULL) {
        queue->first = node;
    }
    else {
        queue->last->next = node;
    }
    queue->last = node;
    VPLQUEUE_ASSERT_INVARIANTS(queue);
}

VPLQueueNode_t* VPLQueue_Dequeue(VPLQueue_t* queue)
{
    VPLQUEUE_ASSERT_INVARIANTS(queue);
    if (queue->first == NULL) {
        return NULL;
    }
    else {
        VPLQueueNode_t* const result = queue->first;
        queue->first = result->next;
        result->next = NULL;
        if (queue->first == NULL) {
            queue->last = NULL;
        }
        VPLQUEUE_ASSERT_INVARIANTS(queue);
        return result;
    }
}

VPLQueueNode_t* VPLQueue_FindAndRemove(VPLQueue_t* queue,
                                      VPLQueueNode_matchFn_t compareFunc,
                                      const void* criteria,
                                      VPL_BOOL removeAll)
{
    VPLQueueNode_t* result = NULL;
    VPLQueueNode_t* curr = queue->first;
    VPLQueueNode_t* prev = NULL;
    VPLQUEUE_ASSERT_INVARIANTS(queue);

    for (;
         curr != NULL;
         prev = curr, curr = curr->next) {

        if (compareFunc(criteria, curr)) {
            /// It matches.
            if (prev == NULL) {
                queue->first = curr->next;
            }
            else {
                prev->next = curr->next;
            }
            /// Check if we're removing the last element.
            if (curr->next == NULL) {
                queue->last = prev;
            }
            curr->next = result;
            result = curr;
            if (!removeAll) {
                /// Found one; stop now.
                break;
            }
        }
    }
    VPLQUEUE_ASSERT_INVARIANTS(queue);
    return result;
}
