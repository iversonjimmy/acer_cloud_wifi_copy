//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_MEM_HEAP_H__
#define __VPLEX_MEM_HEAP_H__

//============================================================================
/// @file
/// Implementation of a memory heap.
/// Manages allocations from the specified block of memory.
//============================================================================

#include "vplu_types.h"
#include "vpl_th.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================
// Public interface
//=============================================

typedef struct VPLMemHeap  VPLMemHeap_t;

VPLMemHeap_t* VPLMemHeap_Init(u32 flags, void* blockOfMemory, size_t size);

int VPLMemHeap_Destroy(VPLMemHeap_t* heap);

void* VPLMemHeap_Alloc(VPLMemHeap_t* heap, size_t size);

int VPLMemHeap_Free(VPLMemHeap_t* heap, void* ptr);

void* VPLMemHeap_Memalign(VPLMemHeap_t* heap, size_t alignment, size_t size);

size_t VPLMemHeap_GetUsableSize(VPLMemHeap_t const* heap);

void VPLMemHeap_GetInfo(VPLMemHeap_t* heap,
                        int* num_free_blocks_out,
                        size_t* bytes_in_free_blocks_out);

//=============================================
// Should treat these as opaque types:
//=============================================
typedef struct _VPLMemHeap_FreeBlk {
    u32 size; //% The size includes the bytes used for this field itself.
    struct _VPLMemHeap_FreeBlk* next;
} VPLMemHeap_FreeBlk_t;

struct VPLMemHeap {
    u32 flags;
    VPLMutex_t mutex;
    VPLMemHeap_t** top;
    VPLMemHeap_FreeBlk_t* bottomFreeBlk;
    VPLMemHeap_FreeBlk_t* topFreeBlk;
    VPLMemHeap_FreeBlk_t* lastToAllocate;
    VPLMemHeap_FreeBlk_t nullBlk; /* never allocated, 0 size first free blk */
};
//=============================================

#ifdef  __cplusplus
}
#endif

#endif // include guard
