#include "e820.h"
#include "km_types.h"
#include "bits.h"
#include "x86.h"
#include "assert.h"
#include "string.h"
#include "printf.h"
#include "bitmap.h"
#include "malloc.h"
#include "x86emu/x86emu.h"
#include "realmode.h"

static E820Entry __e820map[E820MAX] = { };
static int __e820nr;

#define SMAP 0x534D4150

int e820_get(int i, E820Entry *e)
{
    if (i < __e820nr) {
	memcpy(e, &__e820map[i], sizeof *e);
	return 0;
    } else {
	return -1;
    }
}

int e820_nr(void)
{
    return __e820nr;
}

int
e820_init(void)
{
    realmode_t *rm = NULL;
    int ret = -1;
    int count = 0;
    const int dest = 0x20000;

    rm = malloc(sizeof *rm);
    if (rm == NULL) {
	return -1;
    }

    realmode_set_default_callbacks(rm);
    bitmap_init(rm->rm_shadow_bitmap, REALMODE_PAGES, BITMAP_ALL_ONES);

    ret = realmode_create_mem_shadow(rm);
    if (ret < 0) {
	goto out;
    }

#define env (&rm->rm_x86env)

    env->x86.R_CS = 0x1000;
    env->x86.R_SS = 0x1000;
    env->x86.R_ESP = 0xFFF0;
    env->x86.R_IP = 0;
    env->x86.R_EBX = 0;
    
    do {
	env->x86.R_EAX = 0xe820;
	env->x86.R_EDX = SMAP;
	env->x86.R_ECX = sizeof(E820Entry);
	env->x86.R_DS  = dest >> 4;
	env->x86.R_ES  = dest >> 4;
	env->x86.R_DI = 0x0;
	env->x86.R_FLG |= F_CF;

	realmode_int(rm, 0x15);

	if ((env->x86.R_FLG & F_CF) ||
	    env->x86.R_EAX != SMAP ||
	    env->x86.R_ECX != sizeof(E820Entry)) {
	    goto release;
	}

	memcpy(&__e820map[count++], (void *)rm->rm_pages[dest >> VM_PAGE_SHIFT],
	       env->x86.R_ECX);
    } while (env->x86.R_EBX != 0 && count < E820MAX);

    __e820nr = count;
    ret = 0;

 release:
    realmode_release_mem_shadow(rm);
 out:
    free(rm);
    return ret;
}
