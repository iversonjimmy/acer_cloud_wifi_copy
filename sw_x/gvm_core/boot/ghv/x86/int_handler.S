
.macro ENTRY fn
.text
.global \fn
.align 8
\fn:
.endm

#ifdef __x86_64__
#include "int_handler_64.S"
#else
#include "int_handler_32.S"
#endif
