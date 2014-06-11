/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */


#ifndef __ISTORAGE_IMPL_H__
#define __ISTORAGE_IMPL_H__

#include "istorage.h"

//
// The reader/write works on a collection of chunks of memory
// that can be variable sized. The idea here is that data passed into GHV
// from GVM will be sent as a collection of pages. Local data will be
// stored in one contiguous chunk of memory.
//
// This class can handle both types of storage allowing a scattered
// Ticket/TMD/CFM from GVM to be gathered into a contiguous local buffer.
//
// XXX - Could potentially pass in the pfns and map these on the fly...
//

struct iStorage_s {
	u32         is_size;
	u32         is_pos;
	u32	    is_chunk_len;
	u32	    is_chunk_cnt;
	u32	    is_flags;
	u32	   *is_pfns;
#define IS_FLG_NULL	0x00000001
	u8	   *is_chunks[0];	// variable length array
};
typedef struct iStorage_s IStorage;


Result iStoragePfnInit(IStorage **ispp, u32 *pfns, u32 size);
void iStoragePfnFree(IStorage **ispp);
Result iStorageMemInit(IStorage **ispp, void *buf, u32 size);
void iStorageMemFree(IStorage **ispp);
Result iStorageNullInit(IStorage **ispp, u32 size);
void iStorageNullFree(IStorage **ispp);

#endif // __ISTORAGE_IMPL_H__
