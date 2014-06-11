//
//  Copyright (C) 2007-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_mem_heap.h"

#include "vplexTest.h"


void testVPLMemHeap(void)
{
    char buf[1024];
    void *p[3];
    VPLMemHeap_t *h;
    int i;

    h = VPLMemHeap_Init(0 /* not used */, buf, sizeof(buf));
    VPLTEST_LOG("MemHeap: Initialized 1k heap.");
    VPLTEST_ENSURE_NOT_NULL(h, "Result from VPLMemHeap_Init");

    for (i = 0; i < 1000; i++) {
        p[0] = VPLMemHeap_Alloc(h, 20);
        p[1] = VPLMemHeap_Alloc(h, 390);
        p[2] = VPLMemHeap_Alloc(h, 24);


        if (p[0] == NULL || p[1] == NULL || p[2] == NULL) {
            VPLTEST_LOG("MemHeap: Failure to allocate.");
            vplTest_incrErrCount();
            break;
        }

        VPLMemHeap_Free(h, p[1]);
        VPLMemHeap_Free(h, p[2]);
        VPLMemHeap_Free(h, p[0]);
    }

    VPLMemHeap_Destroy(h); 
    VPLTEST_LOG("MemHeap: Destroyed heap.");
}
