/*
 * Copyright 2010 iGware Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 * NON INFRINGEMENT. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 */

#ifndef __HYPERVISOR_H__
#define __HYPERVISOR_H__

/* Linux interface to hypervisor */

#ifdef __KERNEL__

inline static bool
BroadOn_hypervisor_detected(void)
{
	unsigned int a, b, c, d;
	char buf[16];
	const char id[] = "BroadOn";

	if ((cpuid_ecx(1) & 0x80000000) == 0) {
		/* No hypervisor */
		return false;
	}

	/* Now look specifically for BroadOn hypervisor */
	cpuid(0x40000000, &a, &b, &c, &d);
	*(unsigned int *)buf = b;
	*(unsigned int *)(buf + 4) = d;
	*(unsigned int *)(buf + 8) = c;
	buf[12] = '\0';

	return (strncmp(buf, id, strlen(id)) == 0);
}

#endif

inline static long
hypercall0(long num)
{
	long result;
	asm volatile("vmcall" :
		     "=a" (result) :
		     "a" (num) :
		     "memory");
	return result;
}

inline static long
hypercall1(long num, long arg)
{
	long result;
	asm volatile("vmcall" :
		     "=a" (result) :
		     "a" (num), "b" (arg) :
		     "memory");
	return result;
}

inline static long
hypercall2(long num, long arg1, long arg2)
{
	long result;
	asm volatile("vmcall" :
		     "=a" (result) :
		     "a" (num), "b" (arg1), "c" (arg2) :
		     "memory");
	return result;
}

inline static long
hypercall3(long num, long arg1, long arg2, long arg3)
{
	long result;
	asm volatile("vmcall" :
		     "=a" (result) :
		     "a" (num), "b" (arg1), "c" (arg2), "d" (arg3) :
		     "memory");
	return result;
}

inline static long
hypercall4(long num, long arg1, long arg2, long arg3, long arg4)
{
	long result;
	asm volatile("vmcall" :
		     "=a" (result) :
		     "a" (num), "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4) :
		     "memory");
	return result;
}

#endif
