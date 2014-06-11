
#ifndef __IOSLIBC__
#define __IOSLIBC__

#include "km_types.h"

#ifdef _USER_LEVEL
#  include <stdio.h>
#  include <string.h>
#  include <stdlib.h>
#  include <stddef.h>
#  include <core_glue.h>
#elif defined(_KERNEL_MODULE)
// TODO: What goes here?  If nothing, we should put a comment that it's intentional.
#else
#  error "Unsupported configuration"
#endif

#if 0
#include "malloc.h"
#include "string.h"
#endif // 0

#endif // __IOSLIBC__
