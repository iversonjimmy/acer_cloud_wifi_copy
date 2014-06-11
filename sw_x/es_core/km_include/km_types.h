
#ifndef __KM_TYPES_H__
#define __KM_TYPES_H__

#if defined(_USER_LEVEL) && defined(_KERNEL_MODULE)
#error Conflict: defined(_USER_LEVEL) && defined(_KERNEL_MODULE)
#endif

#ifdef _USER_LEVEL

#include "vplu_types.h"
#include "vplu_format.h"

#ifndef _MSC_VER
#include <stdbool.h>
#endif

typedef unsigned long un;
typedef un addr_t;

#endif // _USER_LEVEL

#ifdef _KERNEL_MODULE

#include <linux/types.h>

# define FMT0x32 "0x%08x"
# define FMT0x64 "0x%016llx"

#ifndef NULL
#define NULL 0
#endif

#endif // _KERNEL_MODULE

#endif // include guard
