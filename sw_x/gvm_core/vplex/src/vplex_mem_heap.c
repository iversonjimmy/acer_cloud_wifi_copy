//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_mem_heap.h"
#include "vplex_mem_utils.h"
#include "vplex_safe_conversion.h"
#include "vplex_private.h"

// Size to which we will round-up any allocation requests.
// As a result, the heap is always aligned to this boundary.
#define VPL_MEM_HEAP_MIN_ALLOC_SIZE    (sizeof(size_t))

// Above, as a bitmask...
#define VPL_MEM_HEAP_MIN_ALLOC_MASK    ((VPL_MEM_HEAP_MIN_ALLOC_SIZE) -1)

/*
#define VPL_MEM_HEAP_CHK
#define VPL_MEM_HEAP_0_FILL
#define VPL_MEM_HEAP_FILL
*/

/*
#if defined(DEBUG) || defined(_DEBUG)
    #define VPL_MEM_HEAP_FILL
#endif
*/

#ifdef VPL_MEM_HEAP_0_FILL
    #define VPL_MEM_HEAP_INIT_PATTERN   0
    #define VPL_MEM_HEAP_FREE_PATTERN   0
    #define VPL_MEM_HEAP_ALLOC_PATTERN  0
#elif defined(VPL_MEM_HEAP_CHK) || defined(VPL_MEM_HEAP_FILL)
    #define VPL_MEM_HEAP_INIT_PATTERN   0xFADE0BAD
    #define VPL_MEM_HEAP_FREE_PATTERN   0xDEAF0000
    #define VPL_MEM_HEAP_ALLOC_PATTERN  0xBEEF0000
#else
    // No fill patterns will be used.
#endif


int VPLMemHeap_CheckAllocated(VPLMemHeap_t* heap, void* ptr);
int VPLMemHeap_Check(VPLMemHeap_t* heap);


/**
 * Heap layout:
 * <pre>
 *   heap start:             [pad_bytes_to_align_heap]
 *                           VPLMemHeap_t heap_metadata;
 *                           Free and allocated blocks
 *   "actual_top" of heap:   VPLMemHeap_t* heap_ptr_at_top = &heap_metadata;
 *                           [pad to (u8*)heap + size]
 *   "end"
 * </pre>
 *   The last member of heap_metadata is a 0-size "always free" block: VPLMemHeap_FreeBlk_t  nullBlk.
 */
VPLMemHeap_t* VPLMemHeap_Init(u32 flags, void* blockOfMemory, size_t size)
{
    int errCode;
    /// Round up the bottom to a multiple of #VPL_MEM_HEAP_MIN_ALLOC_SIZE.
    /// "heap_metadata" lives at this address.
    VPLMemHeap_t* h = (VPLMemHeap_t*)SIZE_T_TO_PTR(((PTR_TO_SIZE_T(blockOfMemory) + VPL_MEM_HEAP_MIN_ALLOC_MASK) & (~VPL_MEM_HEAP_MIN_ALLOC_MASK)));
    size_t pad_bytes_to_align_heap = VPLPtr_DifferenceUnsigned(h, blockOfMemory);

    void* end = VPLPtr_AddUnsigned(blockOfMemory, size);
    /// Round down the top to a multiple of #VPL_MEM_HEAP_MIN_ALLOC_SIZE and save room for 
    /// a pointer to "heap_metadata".
    VPLMemHeap_t** actual_top = (VPLMemHeap_t**)SIZE_T_TO_PTR((PTR_TO_SIZE_T(end) - sizeof(h)) & (~VPL_MEM_HEAP_MIN_ALLOC_MASK));
    /// This includes the pointer to "heap_metadata" and any pad bytes.
    size_t overhead_bytes_for_top = VPLPtr_DifferenceUnsigned(end, actual_top);

    size_t heap_overhead = pad_bytes_to_align_heap + sizeof(VPLMemHeap_t) + overhead_bytes_for_top;
    
    /// Due to the overhead, \a size needs to be at least this big to be able to allocate anything.
    size_t min_size = heap_overhead + sizeof(VPLMemHeap_FreeBlk_t) + VPL_MEM_HEAP_MIN_ALLOC_SIZE;

    if ((blockOfMemory == NULL) || (size < min_size)) {
        VPL_LIB_LOG_ERR(VPL_SG_MEM,
                "Failed to init heap, block="FMT0xPTR", size="FMTuSizeT", min size="FMTuSizeT,
                blockOfMemory, size, min_size);
        return NULL;
    }

    h->flags = flags;
    h->top = actual_top;
    *h->top = h;
    h->nullBlk.size = 0;
    h->nullBlk.next = &h->nullBlk + 1;
    
    h->topFreeBlk = &h->nullBlk + 1;
    h->topFreeBlk->size = SIZE_T_TO_U32(size - heap_overhead);
    h->topFreeBlk->next = &h->nullBlk;
    
    h->bottomFreeBlk = &h->nullBlk;
    h->lastToAllocate = &h->nullBlk;

    #ifdef VPL_MEM_HEAP_INIT_PATTERN
    {
        u32*    p = (u32*)(h->nullBlk.next + 1);
        u32     pat = VPL_MEM_HEAP_INIT_PATTERN;
        size_t  num_wds = (h->nullBlk.next->size - sizeof(*h->nullBlk.next))
                            / sizeof(pat);
        u32     i;
        for (i = 0;   i < num_wds;  ++i) {
            p[i] = pat;
        }
    }
    #endif

    // Explicitly set this to avoid accessing truly uninitialized memory.
    VPL_SET_UNINITIALIZED(&h->mutex);
    errCode = VPLMutex_Init(&h->mutex);
    if (errCode != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_MEM, "Failed to init mutex, heap=%p, size="FMTuSizeT", result=%d",
                h, size, errCode);
        h = NULL;
    }

    VPL_LIB_LOG_FINEST(VPL_SG_MEM, "Passed:  block=%p  size="FMTuSizeT"  "
        "result:  heap %p  overhead "FMTuSizeT"  top %p  *top  %p  "
        "free blk:  start %p  size "FMTu32"  next %p  "
        "free space:  start %p  size "FMTuSizeT"  "
        "first pat 0x%x  last pat 0x%x",
        blockOfMemory, size,
        h, heap_overhead, h->top, *h->top,
        h->nullBlk.next, h->nullBlk.next->size, h->nullBlk.next->next,
        &h->nullBlk.next->next,
        h->nullBlk.next->size - sizeof(h->nullBlk.next->size),
        *((u32*)(h->nullBlk.next + 1)), *((u32*)h->top - 1));

    return h;
}

void VPLMemHeap_GetInfo(
        VPLMemHeap_t* heap,
        int* num_free_blocks_out,
        size_t* bytes_in_free_blocks_out)
{
    int rv;
    VPLMemHeap_FreeBlk_t* cur;
    *num_free_blocks_out = 0;
    *bytes_in_free_blocks_out = 0;

    ASSERT_NOT_NULL(heap);

    rv = VPLMutex_Lock(&heap->mutex);
    if (rv != VPL_OK) {
        // Should never occur.
        FAILED_ASSERT("Failed to unlock heap mutex. heap="FMT0xPTR, heap);
    }

    for (cur = heap->nullBlk.next;
         cur != &heap->nullBlk;
         cur = cur->next)
    {
        (*num_free_blocks_out)++;
        *bytes_in_free_blocks_out += cur->size;
    }

    rv = VPLMutex_Unlock(&heap->mutex);
    if (rv != VPL_OK) {
        // Should never occur.
        FAILED_ASSERT("Failed to unlock heap mutex. heap="FMT0xPTR, heap);
    }
}

size_t VPLMemHeap_GetUsableSize(VPLMemHeap_t const* heap)
{
    return VPLPtr_DifferenceUnsigned(heap->top, heap) - sizeof(VPLMemHeap_t);
}

int VPLMemHeap_Destroy(VPLMemHeap_t* heap)
{
    int ec;

    if (heap == NULL) {
        FAILED_ASSERT("Unexpected");
        return VPL_ERR_FAIL;
    }

    ec = VPLMutex_Destroy(&heap->mutex);
    if (ec != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_MEM, "Failed to destroy mutex, heap=%p", heap);
    }

    VPL_LIB_LOG_FINEST(VPL_SG_MEM, "heap=%p, result %d", heap, ec);

    return ec;
}


void* VPLMemHeap_Alloc(VPLMemHeap_t *h, size_t size)
{
    int ec;
    void* ptr;
    VPLMemHeap_FreeBlk_t* prev;
    VPLMemHeap_FreeBlk_t* cur;
    VPLMemHeap_FreeBlk_t* ptrBlk;
   int free_blocks_checked = 0;
   int bytes_in_free_blocks = 0;

    const size_t required =
        ((size + VPL_MEM_HEAP_MIN_ALLOC_MASK) & (~VPL_MEM_HEAP_MIN_ALLOC_MASK)) + offsetof(VPLMemHeap_FreeBlk_t, next);

    ASSERT_NOT_NULL(h);

    if ( (size == 0) || (h == NULL) ) {
        return NULL;
    }

    ec = VPLMutex_Lock (&h->mutex);
    if (ec != VPL_OK) {
        /* Should never occur. */
        VPL_LIB_LOG_ERR(VPL_SG_MEM, "Failed to lock mutex, heap=%p, size="FMTuSizeT,
                 h, size);
        FAILED_ASSERT("%d", ec);
        return NULL;
    }

    #ifdef VPL_MEM_HEAP_CHK
        VPLMemHeap_Check (h);
    #endif

    ptr  = NULL;
    prev = h->lastToAllocate;
    cur  = h->lastToAllocate->next;

    /*
    *   do {  if (cur->size >= (required + sizeof(VPLMemHeap_FreeBlk_t))
    *                   or   cur->size == required ) {
    *             allocate from cur block
    *             break
    *         }
    *
    *         prev = cur;
    *         cur  = cur->next;
    *
    *   } while ( prev != h->lastToAllocate );
    */

    do {
        free_blocks_checked++;
        bytes_in_free_blocks += cur->size;

        if ( (cur->size >= (required + sizeof(VPLMemHeap_FreeBlk_t))) ||
             (cur->size == required)) {

            // Found a suitable free block.

            h->lastToAllocate = prev;
            ptr = &cur->next;
            ptrBlk = cur;

            if (cur->size == required){
                prev->next = cur->next;  /* Use entire block */
            }
            else {
                /* Allocate block from bottom of free block */
                VPLMemHeap_FreeBlk_t* remaining = (VPLMemHeap_FreeBlk_t*)VPLPtr_AddUnsigned(cur, required);
                remaining->size = cur->size - SIZE_T_TO_U32(required);
                cur->size  = SIZE_T_TO_U32(required);
                remaining->next = cur->next;
                prev->next = remaining;
            }

            if (ptrBlk == h->bottomFreeBlk) {
                h->bottomFreeBlk = prev->next;
            }
            else if (ptrBlk == h->topFreeBlk) {
                if (prev->next > prev)
                    h->topFreeBlk = prev->next;
                else
                    h->topFreeBlk = prev;
            }

            break;
        }

        prev = cur;
        cur  = cur->next;

    }  while (prev != h->lastToAllocate);

    if (ptr == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_MEM,
                "Failed to allocate "FMTuSizeT" bytes; found %d bytes in %d free blocks, heap size="FMTuSizeT,
                size, bytes_in_free_blocks, free_blocks_checked, VPLMemHeap_GetUsableSize(h));
        #if defined __GLIBC__ && !defined __UCLIBC__
		VPLStack_Trace();
		#endif
    }
    #ifdef VPL_MEM_HEAP_ALLOC_PATTERN
    else {
        u32     i;
        u32*    p = ptr;
        u32     pat = VPL_MEM_HEAP_ALLOC_PATTERN;
        size_t  num_wds = (required - sizeof cur->size)/sizeof(pat);

        if (pat) {
            pat |= (0x0000FFFF & (size_t) p);
        }

        for (i = 0;   i < num_wds;  ++i) {
            p[i] = pat;
        }
        #ifdef VPL_MEM_HEAP_CHK
            VPLMemHeap_Check (h);
        #endif
    }
    #endif

    ec = VPLMutex_Unlock(&h->mutex);
    if (ec != VPL_OK) {
        /* Should never occur. */
        VPL_LIB_LOG_ERR(VPL_SG_MEM,
                "Failed to unlock heap mutex. heap="FMT0xPTR", size="FMTuSizeT,
                 h, size);
        FAILED_ASSERT_NO_MSG();
    }

    VPL_LIB_LOG_FINEST(VPL_SG_MEM,
                "heap="FMT0xPTR", size="FMTuSizeT", required="FMTuSizeT", result="FMT0xPTR", next="FMT0xPTR,
                h, size, required, ptr, prev->next);

    return ptr;
}

#ifdef VPL_INTERNAL
static void VPLMemHeap_Dump(VPLMemHeap_t* h, VPLTrace_level level)
{
    UNUSED(h);
    UNUSED(level);
    VPL_LIB_LOG(level, VPL_SG_MEM, "heap %p  top %p  *top  %p  "
        "free blk:  start %p  size "FMTu32"  next %p  "
        "free space:  start %p  size "FMTuSizeT"  "
        "first pat 0x%x  last pat 0x%x",
        h, h->top, *h->top,
        h->nullBlk.next, h->nullBlk.next->size, h->nullBlk.next->next,
        &h->nullBlk.next->next,
        h->nullBlk.next->size - sizeof(h->nullBlk.next->size),
        *((u32*)(h->nullBlk.next + 1)), *((u32*)h->top - 1));
    /// TODO: a nice printout like RVL's MEMDumpHeap would be nice.
}
#endif

static int VPLHeap_Free(VPLMemHeap_t* heap, void* ptr)
{
    int ec = VPL_OK;
    int rv = VPL_OK;

    VPLMemHeap_FreeBlk_t* to_free;
    VPLMemHeap_FreeBlk_t* blk_prev_of_freed;

    #ifdef VPL_MEM_HEAP_FREE_PATTERN
        u32*    pats;
        size_t  pat_wds;
        u32     pat = VPL_MEM_HEAP_FREE_PATTERN;
    #endif

    if (heap == NULL) {
        FAILED_ASSERT_NO_MSG();
        return VPL_ERR_FAIL;
    }
    if (ptr == NULL) {
        FAILED_ASSERT_NO_MSG();
        return VPL_ERR_FAIL;
    }

    // Adjust the pointer to point to the actual start of the allocated block.
    to_free = (VPLMemHeap_FreeBlk_t*)VPLPtr_AddSigned(ptr, -(ptrdiff_t)offsetof(VPLMemHeap_FreeBlk_t, next));

    {
        VPLMemHeap_FreeBlk_t* to_free_top = (VPLMemHeap_FreeBlk_t*)VPLPtr_AddUnsigned(to_free, to_free->size);
        if (to_free_top > (VPLMemHeap_FreeBlk_t*) heap->top - 1  &&
               to_free_top != (VPLMemHeap_FreeBlk_t*) heap->top) {

            FAILED_ASSERT("heap=%p, ptr=%p: to_free top %p is above heap top %p",
                                heap, ptr, to_free_top, heap->top);
            return VPL_ERR_FAIL;
        }
    }
    
    ec = VPLMutex_Lock (&heap->mutex);
    if (ec != VPL_OK) {
        /* Should never occur. */
        FAILED_ASSERT("Failed to lock mutex: heap=%p, ec=%d", heap, ec);
        return ec;
    }

    if (to_free <= heap->bottomFreeBlk) {
        /* should never happen in this implementation */
        FAILED_ASSERT("heap=%p, ptr=%p: to_free %p is below heap bottom %p",
                            heap, ptr, to_free, heap->bottomFreeBlk);
        rv = VPL_ERR_FAIL;
        goto end;
    }

    #ifdef VPL_MEM_HEAP_FREE_PATTERN
    {
        #ifdef VPL_MEM_HEAP_CHK
            if ( ((rv = VPLMemHeap_CheckAllocated(heap, ptr)) == VPL_OK) ||
                 ((rv = VPLMemHeap_Check(heap)) == VPL_OK) ) {
                goto end;
            }
        #endif
        pats = (u32*) (to_free + 1);
        pat_wds =  (to_free->size - sizeof *to_free)/sizeof(pat);
    }
    #endif

    if (to_free > heap->topFreeBlk) {


        // Check for adjacent at bottom of to_free.

        VPLMemHeap_FreeBlk_t* top_free_blk_top = 
            (VPLMemHeap_FreeBlk_t*)VPLPtr_AddUnsigned(heap->topFreeBlk, heap->topFreeBlk->size);
        if (top_free_blk_top == to_free) {
            // to_free is adjacent to the highest free block; just add to_free to
            // that block.
            heap->topFreeBlk->size += to_free->size;
            #ifdef VPL_MEM_HEAP_FREE_PATTERN
                pats = (u32*) to_free;
                pat_wds = to_free->size / sizeof(pat);
            #endif
        }
        else {
            // to_free is the new "top free block".
            to_free->next       = heap->bottomFreeBlk;
            heap->topFreeBlk->next = to_free;
            heap->topFreeBlk       = to_free;
        }

        blk_prev_of_freed = heap->topFreeBlk;
    }
    else {
        // The 2nd highest free block below to to_free.
        VPLMemHeap_FreeBlk_t* prev_lower_blk;

        // The highest free block below to to_free.
        VPLMemHeap_FreeBlk_t* lower_blk;

        // The lowest free block above to to_free.
        VPLMemHeap_FreeBlk_t* upper_blk;

        if ( (heap->bottomFreeBlk == NULL) ||
             (heap->topFreeBlk == NULL) ||
             (heap->bottomFreeBlk->next == NULL) ) {
            
            FAILED_ASSERT("heap=%p, ptr=%p: h->bottomFreeBlk=%p,"
                    " h->topFreeBlk=%p, h->bottomFreeBlk->next=%p",
                    heap, ptr, heap->bottomFreeBlk,
                    heap->topFreeBlk, heap->bottomFreeBlk->next);
        }

        prev_lower_blk = heap->topFreeBlk;
        lower_blk      = heap->bottomFreeBlk;
        upper_blk      = lower_blk->next;

        // Search through the free list until we find where to_free belongs.
        while (to_free > upper_blk) {

              if ( (lower_blk == NULL) ||
                   (prev_lower_blk == NULL) ||
                   (upper_blk == NULL) ||
                   (upper_blk->next == NULL) ) {
                  
                  FAILED_ASSERT("heap=%p, ptr=%p: lower_blk=%p,"
                          " prev_lower_blk=%p, upper_blk=%p, upper_blk->next=%p",
                          heap, ptr, lower_blk,
                          prev_lower_blk, upper_blk, upper_blk->next);
              }

              prev_lower_blk = lower_blk;
              lower_blk      = upper_blk;
              upper_blk      = upper_blk->next;
        }


        blk_prev_of_freed = lower_blk;
        lower_blk->next   = to_free;
        to_free->next     = upper_blk;

        /* Check for adjacent at bottom of to_free */
        {
            // This will never coalesce our "null block", since its size of 0
            // keeps this below the first possible to_free location.
            VPLMemHeap_FreeBlk_t* lower_blk_top =
                (VPLMemHeap_FreeBlk_t*)VPLPtr_AddUnsigned(lower_blk, lower_blk->size);

            if (lower_blk_top == to_free) {
                // to_free is adjacent to the previous free block; combine them.
                lower_blk->size  += to_free->size;
                lower_blk->next   = upper_blk; // same as to_free->next
                #ifdef VPL_MEM_HEAP_FREE_PATTERN
                    pats = (u32*) to_free;
                    pat_wds = to_free->size / sizeof(pat);
                #endif
                to_free           = lower_blk;
                blk_prev_of_freed = prev_lower_blk;
            }
        }
        
        // Check for adjacent at top of to_free.
        {
            VPLMemHeap_FreeBlk_t* to_free_top =
                (VPLMemHeap_FreeBlk_t*)VPLPtr_AddUnsigned(to_free, to_free->size);

            if (to_free_top == upper_blk) {
                // to_free is adjacent to the next free block; combine them.
                to_free->size += upper_blk->size;
                to_free->next  = upper_blk->next;

                if (upper_blk == heap->topFreeBlk)
                    heap->topFreeBlk = to_free;

                #ifdef VPL_MEM_HEAP_FREE_PATTERN
                    pat_wds += sizeof *upper_blk / sizeof(pat);
                #endif
            }
        }
    }

    /*  Make allocation always start at last
    *   freed or allocated blk.  Despite intuitive
    *   feelings otherwise, published papers say
    *   studies prove it provides least fragmentation
    *   and fastest response.
    */

    heap->lastToAllocate = blk_prev_of_freed;

    #ifdef VPL_MEM_HEAP_FREE_PATTERN
    {
        u32     i;

        if (pat) {
            pat |= (0x0000FFFF & (size_t) to_free);
        }

        for (i = 0;   i < pat_wds;  ++i) {
            pats[i] = pat;
        }
        #ifdef VPL_MEM_HEAP_CHK
            rv = VPLMemHeap_Check (heap);
        #endif
    }
    #endif

end:
    ec = VPLMutex_Unlock(&heap->mutex);
    if (ec != VPL_OK) {
        /* Should never occur. */
        FAILED_ASSERT("Failed to unlock mutex heap, heap=%p, ptr=%p, result=%d",
            heap, ptr, ec);
    }

    if (rv == VPL_OK) {
        rv = ec;
    }

    VPL_LIB_LOG_FINEST(VPL_SG_MEM, "heap=%p, ptr=%p, result=%d",
            heap, ptr, rv);

    return rv;
}



#ifdef VPL_MEM_HEAP_CHK
/**
DONT USE - It is broken!

The condition at line 329 in VPLHeap_Alloc says that if the block is OK,
dont free it.   So none of good blocks are freed.

If the condition is reversed, the check function would say some good block is bad,
and also not freed.

-Raymond 6/13/08
*/
int  VPLMemHeap_CheckAllocated (VPLMemHeap_t *h, void *ptr)
{
   VPLMemHeap_FreeBlk_t  *blk       = &h->nullBlk + 1;
   VPLMemHeap_FreeBlk_t  *next_free = h->nullBlk.next;
   VPLMemHeap_FreeBlk_t  *ptr_blk   = (VPLMemHeap_FreeBlk_t*) ((u32*)ptr -1);
    bool         isFree;

    if (blk == next_free) {
        isFree = true;
        next_free = blk->next;
    } else {
        isFree = false;
    }

    do {
        if (!isFree && (ptr_blk == blk)) {
            return VPL_OK; // ptr is an allocated blk
        }
        blk = (VPLMemHeap_FreeBlk_t*) ((u8*)blk + blk->size);
        if (blk < next_free) {
            isFree = false;
        } else if (blk == next_free) {
            next_free = blk->next;
            isFree = true;
        } else {
            VPL_LIB_LOG_ERR(VPL_SG_MEM, "heap %p  ptr %p"
                "invalid blk %p or blk size %u",
                h, ptr, blk, blk->size);
            ASSERT(blk <= next_free);
            blk = next_free;
            next_free = blk->next;
            isFree = true;
        }
    } while (blk < (VPLMemHeap_FreeBlk_t*) h->top);

    VPL_LIB_LOG_ERR(VPL_SG_MEM, "heap %p  "
        "ptr %p is not allocated block",  h, ptr);

    return VPL_ERR_FAIL;
}


int  VPLMemHeap_Check  (VPLMemHeap_t *h)
{
    int          rv = VPL_OK;
   VPLMemHeap_FreeBlk_t  *blk       = &h->nullBlk + 1;
   VPLMemHeap_FreeBlk_t  *next_free = h->nullBlk.next;
    u32         *p;
    bool         isFree;
    size_t       num_wds;
    u32          pat;
    u32          i;

    if (blk == next_free) {
        isFree = true;
        next_free = blk->next;
    } else {
        isFree = false;
    }

    while (blk < (VPLMemHeap_FreeBlk_t*) h->top) {
        if (isFree) {
            p = (u32*) (blk + 1);
            if (blk->size < sizeof *blk) {
                VPL_LIB_LOG_ERR(VPL_SG_MEM, "heap %p  "
                    "invalid blk %p or blk size %u",
                     h, blk, blk->size);
                ASSERT(blk->size >= sizeof *blk);
                blk = next_free;
                next_free = blk->next;
                rv = VPL_ERR_FAIL;
                continue;
            }

            if (p == (u32*) h->top) {
                break;
            } else if (*p == VPL_MEM_HEAP_INIT_PATTERN) {
                pat = VPL_MEM_HEAP_INIT_PATTERN;
            } else if ((*p & 0xFFFF0000) == VPL_MEM_HEAP_FREE_PATTERN) {
                pat = VPL_MEM_HEAP_FREE_PATTERN | (u32) (0x0000FFFF & (size_t) blk);
            } else {
                VPL_LIB_LOG_ERR(VPL_SG_MEM, "heap %p  "
                    "free blk %p  size %u  pat not found.  is %u at %p",
                     h, blk, blk->size, *p, p);
                ASSERT(blk->size >= sizeof *blk);
                rv = VPL_ERR_FAIL;
                break;
            }

            num_wds = (blk->size - sizeof *blk)/sizeof(pat);
            for (i = 0;   i < num_wds;   ++i) {
                if (p[i] != pat) {
                    VPL_LIB_LOG_ERR(VPL_SG_MEM, "heap %p  "
                        "free blk %p  size %u  overwritten with %u at %p",
                        h, blk, blk->size, p[i], &p[i]);
                    ASSERT(blk->size >= sizeof *blk);
                    rv = VPL_ERR_FAIL;
                    break;
                }
            }
        }

        blk = (VPLMemHeap_FreeBlk_t*) ((u8*)blk + blk->size);
        if (blk == (VPLMemHeap_FreeBlk_t*) h->top) {
            break;
        } else if (blk < next_free) {
            isFree = false;
        } else if (blk == next_free) {
            next_free = blk->next;
            isFree = true;
        } else {
            VPL_LIB_LOG_ERR(VPL_SG_MEM, "heap %p  "
                "invalid blk %p or blk size %u",
                h, blk, blk->size);
            ASSERT(blk <= next_free);
            blk = next_free;
            next_free = blk;
            isFree = true;
            rv = VPL_ERR_FAIL;
        }
    }

    return rv;
}

#endif


void* VPLMemHeap_Memalign(VPLMemHeap_t* h, size_t alignment, size_t size)
{
    u8 *buf = (u8 *)VPLMemHeap_Alloc(h, alignment + size);
    u8 *aligned_buf;
    if (buf == NULL)
        return NULL;
    aligned_buf = (u8*)SIZE_T_TO_PTR((PTR_TO_SIZE_T(buf) + alignment) & ~(alignment - 1));
    if (aligned_buf != buf) {
        *(s32*)(aligned_buf - offsetof(VPLMemHeap_FreeBlk_t, next)) = (s32)VPLPtr_DifferenceSigned(buf, aligned_buf);  // negative offset
    }
    VPL_LIB_LOG_FINEST(VPL_SG_MEM, "buf=%p aligned_buf=%p size="FMTuSizeT, buf, aligned_buf, size);
    return (void*)aligned_buf;
}

int VPLMemHeap_Free(VPLMemHeap_t* h, void* ptr)
{
    /// Since we return NULL for #VPLMemHeap_Alloc(0), we must allow NULL here.  In other
    /// words, "free(malloc(0))" should never crash.
    if (ptr == NULL) {
        return VPL_OK;
    } else {
        u8 *buf = (u8*) ptr;
        s32 size = *(s32*)(buf - offsetof(VPLMemHeap_FreeBlk_t, next));
        if (size < 0) {
            buf += size;
            VPL_LIB_LOG_FINEST(VPL_SG_MEM, "buf=%p aligned_buf=%p", buf, ptr);
        }
        return VPLHeap_Free(h, buf);
    }
}
