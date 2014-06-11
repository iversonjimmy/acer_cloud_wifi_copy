#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

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
copy_to_hypervisor(un vaddr, un len)
{
	return hypercall2(962, vaddr, len);
}

int
main(int argc, char **argv)
{
	sn ret = 0;

	if (argc < 2) {
		printf("usage: %s <filename>\n", argv[0]);
		return 1;
	}

	char *filename = argv[1];
	printf("filename %s\n", filename);

	int fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("open %s failed: %s\n", filename, strerror(errno));
		return 1;
	}

	struct stat sb;
	ret = fstat(fd, &sb);
	if (ret < 0) {
		printf("fstat %s failed: %s\n", filename, strerror(errno));
		return 1;
	}
	printf("file length 0x%lx\n", sb.st_size);

	un vaddr = (un)mmap(NULL, sb.st_size, PROT_READ,
			    MAP_SHARED|MAP_POPULATE,
			    fd, 0);
	if ((sn)vaddr == -1) {
		printf("mmap %s failed: %s\n", filename, strerror(errno));
		return 1;
	}

	printf("mapped at virtual addr 0x%lx\n", vaddr);

	for (int i = 0; i < sb.st_size; i++) {
		putchar(((char *)vaddr)[i]);
	}

	ret = copy_to_hypervisor(vaddr, sb.st_size);
	if (ret < 0) {
		printf("copy_to_hypervisor() failed: %s\n", strerror(-ret));
	}
}
