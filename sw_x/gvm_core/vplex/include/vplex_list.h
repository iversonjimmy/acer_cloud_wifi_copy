//
//  Copyright (C) 2008-2009, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_LIST_H__
#define __VPLEX_LIST_H__

//============================================================================
/// @file
/// Currently provides a singly-linked list data structure.
//============================================================================

#include "vplex_assert.h"
#include "vpl_types.h"

//% TODO: this was meant to be useful for any type of collection in VPL, not just ListNode.
/// Returns VPL_TRUE if the key and value match, or VPL_FALSE if they do not.
typedef VPL_BOOL (*VPLListNode_matchFn_t)(const void* criteria, const void* candidate_value);

/// Callback type for use when deleting an element from a collection.
typedef void (*VPLListNode_freeFn_t)(void* ptr);

/// A #VPLListNode_matchFn_t that simply compares the two addresses.
VPL_BOOL VPLListNode_MatchByPtr(const void* criteria, const void* candidate_value);

typedef struct VPLListNode {
    struct VPLListNode* next;
    /// Represents the additional fields of each "subclass".
    /// @note Do not use directly; this is just to tell the compiler that this is a
    ///       variable size struct.
    u8 _data[];
} VPLListNode_t;

typedef struct VPLList {
    VPLListNode_t* first;
} VPLList_t;

typedef struct VPLCountedList {
    VPLListNode_t* first;
    u32 count;
} VPLCountedList_t;

//----------------------------------------------------------------------------

/// An initializer intended for assignment to a static variable.
#define VPL_LIST_INITIALIZER  { NULL }

static inline void VPLList_Init(VPLList_t* list_ptr)
{
    list_ptr->first = NULL;
}

static inline
void VPLList_AddNode(VPLList_t* list_ptr, VPLListNode_t* new_node)
{
    VPLListNode_t* old_first_node = list_ptr->first;
    new_node->next = old_first_node;
    list_ptr->first = new_node;
}

static inline
void VPLList_AddNodeUntyped(VPLList_t* list_ptr, void* new_node)
{
    VPLList_AddNode(list_ptr, (VPLListNode_t*)new_node);
}

/// Remove and return the most recently added node.
static inline
VPLListNode_t* VPLList_PopNode(VPLList_t* list_ptr)
{
    VPLListNode_t* old_first_node = list_ptr->first;
    list_ptr->first = list_ptr->first->next;
    old_first_node->next = NULL;
    return old_first_node;
}

static inline
VPL_BOOL VPLList_IsEmpty(VPLList_t* list_ptr)
{
    return list_ptr->first == NULL;
}

VPLListNode_t* VPLList_FindNode(VPLList_t* list_ptr,
                                VPLListNode_matchFn_t match_func,
                                const void* criteria);

VPLListNode_t* VPLList_RemoveNode(VPLList_t* list_ptr,
                                      VPLListNode_matchFn_t match_func,
                                      const void* criteria);

/// TEMP: when compiled for release, I want to see how different this alternate implementation is in binary.
VPLListNode_t* VPLList_RemoveNode2(VPLList_t* list_ptr,
                                   VPLListNode_matchFn_t match_func,
                                   const void* criteria);

/// Remove all elements from the list, calling \a free_func on each one.
void VPLList_Clear(VPLList_t* list_ptr,
                    VPLListNode_freeFn_t free_func);

/// Remove and call \a free_func for any elements in the list that match the criteria.
/// @note The candidates passed to \a match_func will be #VPLListNode_t pointers.
void VPLList_DeleteMulti(VPLList_t* list_ptr,
                           VPLListNode_matchFn_t match_func,
                           const void* criteria,
                           VPLListNode_freeFn_t free_func);

//----------------------------------------------------------------------------

/// Iterates over the list; "for each node in list".
/// @note Does not support removal of elements while iterating; use 
///        #VPLListIterator_Get() and #VPLListIterator_GetNext()
///        if elements may be removed.
#define foreach_VPLList(node, list_ptr)     \
    for (node = (void*)((list_ptr)->first);  \
         node != NULL;                       \
         node = (void*)((node)->next))

typedef struct VPLListIterator {
    VPLListNode_t* next_to_return;
} VPLListIterator_t;

/// Allows iterating over a list and supports removing elements while iterating.
static inline
VPLListIterator_t VPLListIterator_Get(VPLList_t* list_ptr)
{
    VPLListIterator_t result = { list_ptr->first };
    return result;
}

/**
 * \n Example usage: \code
    VPLListIterator_t foo_iter = VPLListIterator_Get(&foo_list);
    foo_t* curr_foo;
    while ( (curr_foo = VPLListIterator_GetNext(&foo_iter)) != NULL ) {
        ...
    }
\endcode */
static inline
void* VPLListIterator_GetNext(VPLListIterator_t* iterator_ptr)
{
    void* result = iterator_ptr->next_to_return;
    if (result != NULL) {
        iterator_ptr->next_to_return = iterator_ptr->next_to_return->next;
    }
    return result;
}

#endif // include guard
