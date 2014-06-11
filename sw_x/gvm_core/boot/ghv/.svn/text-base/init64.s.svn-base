	.section .init
_init:	
	.global _init
	push %rbp
	mov %rsp, %rbp
	mov $_kern_temp_stack_end, %rsp
	call main
	leave
	ret
	
	.text
	.global _usermode_exit
_usermode_exit:
	mov %rax, %rbx
	xor %rax, %rax
	int $0x90
1:	jmp 1b

	.bss
	.align 0x1000
_kern_temp_stack:	
	.space 0x2000
_kern_temp_stack_end:
