//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished proprietary information of BroadOn Communications Corp.
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX__PLAT_H__
#define __VPLEX__PLAT_H__

#ifndef __cplusplus
#include <stdbool.h>    // ISO C99 boolean type
#endif

#include <stdlib.h>
#ifdef ANDROID
// Android doesn't currently have posix_memalign.
static inline
int posix_memalign(void** memptr, size_t alignment, size_t size)
{
    *memptr = memalign(alignment, size);
    return (*memptr == NULL) ? -1 : 0;
}
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define VPL_PLAT_HAS_CALENDAR_TIME  1

// TODO
// Using basename(__FILE__) is bad because we don't want __FILE__,
// which may contain directory information outside of the source
// tree, exposed in rodata.
// We should probably run GCC from the directory of the source file.
// Alternatively, there's a patch at http://gcc.gnu.org/bugzilla/show_bug.cgi?id=42579
#define _FILE_NO_DIRS_  __FILE__

#define VPL_PLAT_SYSLOG_IMPL  1
#define VPL_SYSLOG_ENABLED defined
#define USE_SYSLOG  1

#define VPL_DEFAULT_USER_NAMESPACE  "acer"

#ifdef __cplusplus
}
#endif

#endif // include guard
