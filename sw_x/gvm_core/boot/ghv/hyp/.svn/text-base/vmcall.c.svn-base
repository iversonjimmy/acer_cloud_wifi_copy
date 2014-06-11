#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

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

	if (num > 0) {
		switch (argc) {
		case 5:
			arg3 = atoi(argv[4]);
			arg2 = atoi(argv[3]);
			arg1 = atoi(argv[2]);
			ret = hypercall3(num, arg1, arg2, arg3);
			break;
		case 4:
			arg2 = atoi(argv[3]);
			arg1 = atoi(argv[2]);
			ret = hypercall2(num, arg1, arg2);
			break;
		case 3:
			arg1 = atoi(argv[2]);
			ret = hypercall1(num, arg1);
			break;
		default:
			ret = hypercall0(num);
			break;
		}
		if (ret) {
			printf("vmcall returned 0x%lx\n", ret);
		}
		return 0;
	}

	/* loopcount must be 1 million, or ns calculation is incorrect */
	un loopcount = 1000000;
	un freq_kHz = hypercall0(901);

	printf("vmcall enter\n");
	u64 start = rdtsc();

	for (un i = 0; i < loopcount; i++) {
		hypercall0(num);
	}

#define cycles_to_ms(x) ((x) / freq_kHz)

	u64 finish = rdtsc();

	printf("vmcall took %lld cycles (%lld ns)\n",
	       (finish - start)/loopcount,
	       cycles_to_ms(finish - start));

#ifndef _WIN32
	printf("getpid enter\n");
	start = rdtsc();

	for (un i = 0; i < loopcount; i++) {
		syscall(20);
	}

	finish = rdtsc();

	printf("getpid took %lld cycles (%lld ns)\n",
	       (finish - start)/loopcount,
	       cycles_to_ms(finish - start));
#endif
	return 0;
}
