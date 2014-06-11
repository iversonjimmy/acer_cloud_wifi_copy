// #define EC_DEBUG	1

#include "istorage_impl.h"

#include "ec_common.h"

//
// Multiple types of init functions possible...
//
static IStorage *
_iStorageAlloc(u32 chunk_cnt)
{
	size_t	len;
	IStorage *isp;

	len = sizeof(IStorage) + (chunk_cnt * sizeof(u8*));
	isp = (IStorage *)malloc(len);
	memset(isp, 0, len);
	isp->is_flags = 0;

	if ( isp ) {
		isp->is_chunk_cnt = chunk_cnt;
		isp->is_pos = 0;
	}

	return isp;
}

static void
_iStorageFree(IStorage **ispp)
{
	free(*ispp);
	*ispp = NULL;
}

Result
iStoragePfnInit(IStorage **ispp, u32 *pfns, u32 size)
{
	IStorage *isp;
	u32	chunk_cnt;

	chunk_cnt = (size + VM_PAGE_SIZE - 1) / VM_PAGE_SIZE;
	
	isp = _iStorageAlloc(chunk_cnt);
	if ( !isp ) {
		return -1;
	}

	DBLOG("Chunk cnt %d\n", chunk_cnt);
	isp->is_pfns = pfns;
	isp->is_size = size;
	isp->is_chunk_len = VM_PAGE_SIZE;

	*ispp = isp;

	return 0;
}

void
iStoragePfnFree(IStorage **ispp)
{
	// free up the structure
	_iStorageFree(ispp);
}

//
// Single chunk that contains the whole thing
//
Result
iStorageMemInit(IStorage **ispp, void *buf, u32 size)
{
	IStorage *isp;

	isp = _iStorageAlloc(1);
	if ( !isp ) {
		return -1;
	}

	isp->is_chunks[0] = buf;

	isp->is_size = isp->is_chunk_len = size;

	*ispp = isp;

	return 0;
}

void
iStorageMemFree(IStorage **ispp)
{
	// free up the structure
	_iStorageFree(ispp);
}

//
// The null storage is the equivalent of a /dev/null
//
Result
iStorageNullInit(IStorage **ispp, u32 size)
{
	IStorage *isp;

	// This is a special case of the memory device
	if ( iStorageMemInit(&isp, NULL, size) ) {
		return -1;
	}

	// This is a null device
	isp->is_flags = IS_FLG_NULL;

	*ispp = isp;

	return 0;
}

void
iStorageNullFree(IStorage **ispp)
{
	// free up the structure
	_iStorageFree(ispp);
}

static Result
_iStorageIO(IStorage *isp, u32 *pOut, u8 *buf_out, const u8 *buf_in,
	    u32 size, bool isRead)
{
	u32 bytes;
	u32 cnt;
	u32 chunk;
	u32 chunk_off;
	u32 cpy_cnt;
	u32 chunk_avail;

	// Sanity check
	if ( isRead && (isp->is_flags & IS_FLG_NULL) ) {
		return -1;
	}

	bytes = (isp->is_pos + size > isp->is_size)
		? isp->is_size - isp->is_pos
		: size;

	// this loop can be optimized
	for( cnt = bytes ; cnt ; cnt -= cpy_cnt ) {

		chunk = isp->is_pos / isp->is_chunk_len;
		chunk_off = isp->is_pos % isp->is_chunk_len;
		chunk_avail = isp->is_chunk_len - chunk_off;

		cpy_cnt = (cnt > chunk_avail) ? chunk_avail : cnt;

		// If this is a pfn map, map in the page
		if ( isp->is_pfns ) {
			if ( pfn_map((void **)&isp->is_chunks[chunk],
				     isp->is_pfns[chunk]) ) {
				DBLOG("Failed pfn map!\n");
				return -1;
			}
		}

		if ( !(isp->is_flags & IS_FLG_NULL) &&
		     (isp->is_chunks[chunk] == 0) ) {
			DBLOG_N("Invalid chunk pointer - chunk %d", chunk);
			return -1;
		}

		if ( isRead ) {
    			memcpy(buf_out, isp->is_chunks[chunk]+chunk_off,
			       cpy_cnt);
			buf_out += cpy_cnt;
		}
		else if ( !(isp->is_flags & IS_FLG_NULL) ) {
    			memcpy(isp->is_chunks[chunk]+chunk_off, buf_in,
			       cpy_cnt);
			buf_in += cpy_cnt;
		}

		if ( isp->is_pfns ) {
			pfn_unmap(isp->is_pfns[chunk], 0);
			isp->is_chunks[chunk] = NULL;
		}

    		isp->is_pos += cpy_cnt;
	}
	
    	*pOut = bytes;

    	return 0;
}

static Result
_iStoragePositionGet(IStorage *isp, s64 *pOut)
{
    *pOut = isp->is_pos;
    return 0;
}

static Result
_iStorageSizeGet(IStorage *isp, s64 *pOut)
{
    *pOut = isp->is_size;
    return 0;
}

static Result
_iStorageSeek(IStorage *isp, s64 position)
{
    if (position > isp->is_size) {
        return -1;
    }

    isp->is_pos = position;
    return 0;
}

Result
rdrTryRead(IInputStream *rdr, u32 *pOut, void *buffer, u32 size)
{
	return _iStorageIO((IStorage *)rdr, pOut, buffer, NULL, size, true);
}

Result
rdrTrySetPosition(IInputStream *rdr, s64 position)
{
	return _iStorageSeek((IStorage *)rdr, position);
}

Result
rdrTryGetPosition(IInputStream *rdr, s64 *pOut)
{
	return _iStoragePositionGet((IStorage *)rdr, pOut);
}
    
Result
rdrTryGetSize(IInputStream *rdr, s64 *pOut)
{
	return _iStorageSizeGet((IStorage *)rdr, pOut);
}

Result
wrtrTryWrite(IOutputStream *wrtr, u32 *pOut, const void *buf_in, u32 size)
{
	return _iStorageIO((IStorage *)wrtr, pOut, NULL, buf_in, size, false);
}

Result
wrtrTrySetPosition(IOutputStream *wrtr, s64 position)
{
	return _iStorageSeek((IStorage *)wrtr, position);
}

Result
wrtrTryGetPosition(IOutputStream *wrtr, s64 *pOut)
{
	return _iStoragePositionGet((IStorage *)wrtr, pOut);
}

Result
wrtrTryGetSize(IOutputStream *wrtr, s64 *pOut)
{
	return _iStorageSizeGet((IStorage *)wrtr, pOut);
}
