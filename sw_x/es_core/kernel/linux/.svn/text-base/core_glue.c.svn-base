
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/highmem.h>
#include <linux/vmalloc.h>
#include <linux/list.h>

#include "core_glue.h"

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
	*page = kmap(pfn_to_page(pfn));

	return 0;
}

int
pfns_map(void *pages[], u32 *pfns, u32 count)
{
	u32 i;

	for( i = 0 ; i < count ; i++ ) {
		if ( pfns[i] == 0 ) {
			pages[i] = NULL;
			continue;
		}

		pages[i] = kmap(pfn_to_page(pfns[i]));
	}

	return 0;
}

void
pfns_unmap(u32 *pfns, u32 count)
{
	u32 i;

	for( i = 0 ; i < count ; i++ ) {
		if ( pfns[i] == 0 ) {
			continue;
		}
		kunmap(pfn_to_page(pfns[i]));
	}
}

void
pfn_unmap(u32 pfn, un __ump_flags)
{
	kunmap(pfn_to_page(pfn));
}

// This doesn't really belong with the others
u32
heap_free_size(void)
{
	return 0;
}

/* track all allocations so we can clean up later */
struct alloc_rec {
	struct list_head list;
	void *ptr;
	void (*free)(const void *ptr);
};
static struct list_head alloc_list;
static spinlock_t alloc_list_lock;

static void *
alloc(size_t size, gfp_t flags)
{
	void *ptr = NULL;
	struct alloc_rec *an = kmalloc(sizeof(*an), GFP_KERNEL);
	if (!an) return NULL;
	INIT_LIST_HEAD(&an->list);

	if (size > PAGE_SIZE) {
		an->free = vfree;
		an->ptr = ptr = __vmalloc(size, GFP_KERNEL | __GFP_HIGHMEM | flags, PAGE_KERNEL);
	}
	else {
		an->free = kfree;
		an->ptr = ptr = kmalloc(size, GFP_KERNEL | flags);
	}
	if (ptr) {
		spin_lock(&alloc_list_lock);
		list_add(&an->list, &alloc_list);
		spin_unlock(&alloc_list_lock);
	}
	else
		kfree(an);
	return ptr;
}

void *
malloc(size_t size)
{
	return alloc(size, 0);
}

void *
calloc(size_t nmemb, size_t size)
{
	size_t total_size = nmemb * size;
	return alloc(total_size, __GFP_ZERO);
}

void
free(void *ptr)
{
	struct alloc_rec *rec;
	spin_lock(&alloc_list_lock);
	list_for_each_entry(rec, &alloc_list, list) {
		if (rec->ptr == ptr) {
			rec->free(ptr);
			list_del(&rec->list);
			kfree(rec);
			goto end;
		}
	}
 end:
	spin_unlock(&alloc_list_lock);
}

void core_glue_init(void)
{
	INIT_LIST_HEAD(&alloc_list);
	spin_lock_init(&alloc_list_lock);
}

#if 0
void core_glue_dump(void)
{
	spin_lock(&alloc_list_lock);
	if (list_empty(&alloc_list))
		printk(KERN_INFO "alloc_list empty");
	else
		printk(KERN_INFO "alloc_list NOT empty");
	spin_unlock(&alloc_list_lock);
}
#endif

void core_glue_exit(void)
{
	struct alloc_rec *rec, *tmp;
	//core_glue_dump();
	spin_lock(&alloc_list_lock);
	list_for_each_entry_safe(rec, tmp, &alloc_list, list) {
		rec->free(rec->ptr);
		list_del(&rec->list);
		kfree(rec);
	}
	spin_unlock(&alloc_list_lock);
	//core_glue_dump();
}

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
