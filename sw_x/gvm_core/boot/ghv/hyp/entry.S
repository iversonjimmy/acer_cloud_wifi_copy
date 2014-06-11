
.macro ENTRY fn
.text
.global \fn
.align 8
\fn:
.endm

#ifdef __x86_64__
#include "entry64.S"
#else
#include "entry32.S"
#endif
