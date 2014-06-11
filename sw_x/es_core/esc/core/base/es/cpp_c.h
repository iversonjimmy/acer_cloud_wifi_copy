
#ifndef __CPP_C_H__
#define __CPP_C_H__

#ifdef __cplusplus
#define __REFVSPTR	&
#define __DEREFVSPTR	&
#define __REFPASSPTR	*
#else
#define __REFVSPTR	*
#define __DEREFVSPTR
#define __REFPASSPTR
#ifndef _KERNEL_MODULE
// Don't include compiler-supplied stdbool.h for kernel module builds, 
// as it conflicts with bool definition in linux/stddef.h.

#ifdef _MSC_VER
#ifndef bool
#define bool int
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#else
#include <stdbool.h>
#endif

#endif
#endif

#endif // __CPP_C_H__

