//
//  Copyright (C) 2008-2009, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_list.h"

VPL_BOOL VPLListNode_MatchByPtr(const void* criteria, const void* candidate_value)
{
    return (criteria == candidate_value);
}

VPLListNode_t* VPLList_RemoveNode(VPLList_t* list_ptr,
                                      VPLListNode_matchFn_t match_func,
                                      const void* criteria)
{
    VPLListNode_t** prev_next_ptr = &list_ptr->first;
    while (*prev_next_ptr != NULL) {
        VPLListNode_t* const curr = *prev_next_ptr;
        if (match_func(criteria, curr)) {
            *prev_next_ptr = curr->next;
            curr->next = NULL;
            return curr;
        }
        prev_next_ptr = &curr->next;
    }
    return NULL;
}

VPLListNode_t* VPLList_RemoveNode2(VPLList_t* list_ptr,
                                       VPLListNode_matchFn_t match_func,
                                       const void* criteria)
{
    VPLListNode_t* curr = list_ptr->first;
    VPLListNode_t* prev = NULL;
    while (curr != NULL) {
        if (match_func(criteria, curr)) {
            if (prev != NULL) {
                prev->next = curr->next;
            } else {
                list_ptr->first = curr->next;
            }
            curr->next = NULL;
            return curr;
        }
        prev = curr;
        curr = curr->next;
    }
    return NULL;
}

void VPLList_Clear(VPLList_t* list_ptr,
                    VPLListNode_freeFn_t free_func)
{
    while (list_ptr->first != NULL) {
        VPLListNode_t* to_delete = list_ptr->first;
        list_ptr->first = to_delete->next;
        free_func(to_delete);
        // "to_delete" is no longer safe to access.
    }
}

void VPLList_DeleteMulti(VPLList_t* list_ptr,
                           VPLListNode_matchFn_t match_func,
                           const void* criteria,
                           VPLListNode_freeFn_t free_func)
{
    // The previous element's "next" pointer (or the "first in list" pointer).
    VPLListNode_t** ptr_to_curr = &list_ptr->first;
    while (*ptr_to_curr != NULL) {
        VPLListNode_t* curr = *ptr_to_curr;
        if (match_func(criteria, curr)) {
            // We're going to remove "curr".  Update the previous element's
            // next pointer, but don't advance.  (We may update the previous
            // element's next pointer multiple times if "curr->next" also gets removed).
            *ptr_to_curr = curr->next;
            curr->next = NULL;
            free_func(curr);
            // "curr" is no longer safe to access.
        } else {
            // "curr" gets to stay; just advance the pointer to check the next element.
            ptr_to_curr = &curr->next;
        }
    }
}

VPLListNode_t* VPLList_FindNode(VPLList_t* list_ptr,
                                VPLListNode_matchFn_t match_func,
                                const void* criteria)
{
    VPLListNode_t* curr_candidate = list_ptr->first;
    for (;
         curr_candidate != NULL;
         curr_candidate = curr_candidate->next) {

        if (match_func(criteria, curr_candidate)) {
            return curr_candidate;
        }
    }
    return NULL;
}
