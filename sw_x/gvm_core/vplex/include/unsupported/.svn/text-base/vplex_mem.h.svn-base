//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_MEM_H__
#define __VPLEX_MEM_H__

#error "This is not currently supported"

//============================================================================
/// @file
/// Provides an abstraction for platforms such as the Wii, where there are
/// multiple processors and different areas of memory that are either shared
/// or private.
//============================================================================


#include "vplex__mem.h"

#ifdef __cplusplus
extern "C" {
#endif

// Declare a static heap for memory allocation.
#define VPLMEM_DECLARE_STATIC_HEAP(name, size) static VPLMEM__DECLARE_STATIC_HEAP(name, size)
#define VPLMEM_HEAP_INVALID VPLMEM__HEAP_INVALID
#define VPLMEM_SHARED_HEAP_INVALID VPLMEM__SHARED_HEAP_INVALID

// Types for heap IDs.
typedef VPLMem__heapId_t VPLMem_heapId_t;
typedef VPLMem__sharedHeapId_t VPLMem_sharedHeapId_t;

/// Initialize a regular memory heap.
/// @param buf Buffer to turn into a heap.
/// @param size Size of the buffer.
/// @return ID of the initialized heap, or NULL if failed.
VPLMem_heapId_t VPLMem_Init(void* buf, size_t size);

/// Initialize a shared memory heap.
/// @param buf Buffer to turn into a heap.
/// @param size Size of the buffer.
/// @return ID of the initialized heap, or NULL if failed.
VPLMem_sharedHeapId_t VPLMem_InitShared(void* buf, size_t size);

/// Cleanup a regular memory heap.
/// @param heap ID of the regular memory heap.
/// @return VPL_OK on success, relevant negative error code otherwise.
int VPLMem_Cleanup(VPLMem_heapId_t heap);

/// Cleanup a shared memory heap.
/// @param heap ID of the shared memory heap.
/// @return VPL_OK on success, relevant negative error code otherwise.
int VPLMem_CleanupShared(VPLMem_sharedHeapId_t heap);

/// Allocate memory from regular memory heap.
/// @param heap Regular memory heap to allocate from.
/// @param size Size to allocate.
/// @return Pointer to allocated memory on success. NULL on failure.
void* VPLMem_Alloc(VPLMem_heapId_t heap, size_t size);

/// Allocate memory from regular memory heap with alignment
/// @param heap Regular memory heap to allocate from.
/// @param size Size to allocate.
/// @param alignment Byte boundary to align to.
/// @return Pointer to allocated memory on success. NULL on failure.
void* VPLMem_AllocAligned(VPLMem_heapId_t heap, size_t size,
                          u32 alignment);

/// Free memory back to regular memory heap.
/// @param heap Regular memory heap to allocate from.
/// @param ptr Pointer to memory to free.
/// @return VPL_OK on success, relevant negative error code otherwise.
int VPLMem_Free(VPLMem_heapId_t heap, void* ptr);

/// Allocate memory from shared memory heap.
/// @param heap Shared memory heap to allocate from.
/// @param size Size to allocate.
/// @return Pointer to allocated memory on success. NULL on failure.
void* VPLMem_AllocShared(VPLMem_sharedHeapId_t heap, size_t size);

/// Allocate memory from shared memory heap.
/// @param heap Shared memory heap to allocate from.
/// @param size Size to allocate.
/// @param alignment Byte boundary to align to.
/// @return Pointer to allocated memory on success. NULL on failure.
void* VPLMem_AllocSharedAligned(VPLMem_sharedHeapId_t heap, size_t size,
                                u32 alignment);

/// Free memory back to regular memory heap.
/// @param heap Shared memory heap to allocate from.
/// @param ptr Pointer to memory to free.
/// @return VPL_OK on success, relevant negative error code otherwise.
int VPLMem_FreeShared(VPLMem_sharedHeapId_t heap, void* ptr);

#ifdef  __cplusplus
}
#endif

#endif // include guard
