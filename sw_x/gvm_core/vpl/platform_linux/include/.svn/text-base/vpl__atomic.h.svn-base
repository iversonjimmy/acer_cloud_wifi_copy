#ifndef VPL_LINUX_ATOMIC_H__
#define VPL_LINUX_ATOMIC_H__

#include "../src/vpl_assert.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#ifdef VPL_PLAT_IS_X86
static inline int32_t VPLAtomic_add(volatile int32_t* operand, int32_t incr)
{
    int32_t result;
    asm volatile (
        "lock xaddl %0, (%1)\n" // add incr to operand
        : "=r" (result)         
        : "r" (operand), "0" (incr)
        : "cc"
    );
    return result + incr;
}
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // include guard
