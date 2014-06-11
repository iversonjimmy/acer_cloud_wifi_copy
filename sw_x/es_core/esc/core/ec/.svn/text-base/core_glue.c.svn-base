#include "core_glue.h"

#include "ec_common.h"

//
// This is a little tricky. For the user-level version we assume
// that the pfns are actually user-level virtual addresses. There is
// no mapping going on.
//
// This
// code is broken for 64-bit user-level code.
// 
//

int
pfn_map(void **page, u32 pfn)
{
	*page = (void *)pfn;

	return 0;
}

int
pfns_map(void *pages[], u32 *pfns, u32 count)
{
	memcpy(pages, pfns, count * sizeof(u32));

	return 0;
}

void
pfns_unmap(u32 *pfns, u32 count)
{
	// There's nothing to unmap.
}

void
pfn_unmap(u32 pfn, un __ump_flags)
{
	// There's nothing to do here.
}

// This doesn't really belong with the others
u32
heap_free_size(void)
{
	return 0;
}

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
