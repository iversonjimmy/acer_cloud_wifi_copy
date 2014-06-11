
#ifndef __CORE_GLUE_H__
#define __CORE_GLUE_H__

#include "km_types.h"

#ifdef _USER_LEVEL
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define kprintf	printf
#else // !_USER_LEVEL

void kprintf(const char *f, ...) __attribute__ ((format (printf, 1, 2)));

#endif // _USER_LEVEL

#ifdef _KERNEL_MODULE
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);
#define printf(f, args...) printk(KERN_INFO f, ## args)
typedef void* addr_t;
typedef u32 un;
#endif // _KERNEL_MODULE

int pfn_map(void **page, u32 pfn);
int pfns_map(void *pages[], u32 *pfns, u32 count);
void pfns_unmap(u32 *pfns, u32 count);
u32 heap_free_size(void);
void pfn_unmap(u32 pfn, un __ump_flags);

#endif // __CORE_GLUE_H__
