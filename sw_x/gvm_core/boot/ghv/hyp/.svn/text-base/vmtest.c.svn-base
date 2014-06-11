#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "hypervisor.h"

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long		un;

typedef char			s8;
typedef short			s16;
typedef int			s32;
typedef long			sn;


#ifdef __x86_64__
typedef unsigned long		u64;
typedef long			s64;
#else
typedef unsigned long long	u64;
typedef long long		s64;
#endif

inline static u64
rdtsc(void)
{
	u32 eax, edx;

	asm volatile("rdtsc" : "=a" (eax), "=d" (edx));
	return ((u64)edx << 32) | eax;
}

inline static un
get_test_page_addr(void)
{
	return (un)hypercall0(970);
}

inline static void
protect_test_page(void)
{
	hypercall0(971);
}

inline static void
unprotect_test_page(void)
{
	hypercall0(972);
}

inline static void
protect_test_page_no_tlb_flush(void)
{
	hypercall0(973);
}

inline static void
unprotect_test_page_no_tlb_flush(void)
{
	hypercall0(974);
}

int
main(int argc, char **argv)
{
	int num = 0;
	sn arg1 = 0;
	sn arg2 = 0;
	sn arg3 = 0;
	sn ret = 0;

	if (argc > 1) {
		num = atoi(argv[1]);
	}

	un paddr = get_test_page_addr();
	printf("test page phys addr 0x%lx\n", paddr);

	int fd = open("/dev/mem", O_RDWR);
	if (fd < 0) {
		printf("open /dev/mem failed: %s\n", strerror(errno));
		return 1;
	}

	un vaddr = (un)mmap(NULL, 4096, PROT_EXEC|PROT_READ|PROT_WRITE,
			    MAP_SHARED,
			    fd, paddr);
	if ((sn)vaddr == -1) {
		printf("mmap /dev/mem failed: %s\n", strerror(errno));
		return 1;
	}

	printf("accessing test page virtual addr 0x%lx\n", vaddr);
	for (un i = 0; i < 5; i++) {
		*(u32 *)vaddr += 1;
		sleep(1);
	}

	printf("protecting test page virtual addr 0x%lx\n", vaddr);
	protect_test_page();

	printf("attempting to access test page virtual addr 0x%lx\n", vaddr);
	for (un i = 0; i < 5; i++) {
		*(u32 *)vaddr += 1;
		sleep(1);
	}

	unprotect_test_page();
	printf("attempting to access test page virtual addr 0x%lx\n", vaddr);
	for (un i = 0; i < 5; i++) {
		*(u32 *)vaddr += 1;
		sleep(1);
	}

	printf("protecting test page (no TLB flush) virtual addr 0x%lx\n",
	       vaddr);
	protect_test_page_no_tlb_flush();

	printf("attempting to access test page virtual addr 0x%lx\n", vaddr);
	for (un i = 0; i < 5; i++) {
		*(u32 *)vaddr += 1;
		sleep(1);
	}

	unprotect_test_page();
	protect_test_page_no_tlb_flush();
	*(u32 *)vaddr += 1;
	protect_test_page_no_tlb_flush();
	*(u32 *)vaddr += 1;
	protect_test_page_no_tlb_flush();
	*(u32 *)vaddr += 1;
	protect_test_page_no_tlb_flush();
	*(u32 *)vaddr += 1;
	protect_test_page_no_tlb_flush();
	*(u32 *)vaddr += 1;
	protect_test_page_no_tlb_flush();
	*(u32 *)vaddr += 1;
	protect_test_page_no_tlb_flush();
	*(u32 *)vaddr += 1;

	return 0;
}
