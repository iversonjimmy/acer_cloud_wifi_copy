	.section .data.trampoline
_base:
	.code16
_start16:
	wbinvd
	cli

	xorl %eax, %eax
	xorl %ecx, %ecx
	xorl %edx, %edx
	xorl %ebx, %ebx
	xorl %esp, %esp
	xorl %ebp, %ebp
	xorl %esi, %esi
	xorl %edi, %edi

	mov %cs, %bx
	mov %bx, %ds
	shll $4, %ebx				# %ebx is linear address base

	lgdtl _gdt_ptr-_base			# load gdt

	movl _cr3_32-_base, %eax		# linear address of 32-bit PD
	movl %eax, %cr3				# load cr3

	movl $(1<<4), %eax			# enable PSE in %cr4
	movl %eax, %cr4
	
	movl $0x80000001, %eax			# enable PE and PG in one shot
	movl %eax, %cr0

	ljmpl *_ljmp32_ptr-_base		# ljmp to 32-bit
	
	.code32
	.align 8, 0x90
_start_32:
	movl $0x18, %eax			# go linear!
	mov %eax, %ds
	mov %eax, %ss

	xorl %eax, %eax				# nullify unused segments
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs

	# Section 9.8.5 of Intel Vol-3
	# When enabling LM, you must do these things, IN ORDER:
	# 1. Disable paging
	# 2. Set CR4.PAE = 1
	# 3. Load CR3 with PML4
	# 4. Set IA32_EFER.LME = 1
	# 5. Set CR0.PG = 1
	
	movl $1, %eax				# disable paging
	movl %eax, %cr0

	movl $(1<<5)|(1<<7), %eax		# PAE, PGE enable
	movl %eax, %cr4				#

	movl _cr3_64-_base(%ebx), %eax		# load 64-bit pagetables
	movl %eax, %cr3				#

	mov $0xC0000080, %ecx			# IA32_EFER
	rdmsr					#
	or $(1<<8), %eax			# LM enable, when paging is on
	orl _ia32_efer-_base(%ebx), %eax	# enable NXE if available
	wrmsr
	
	movl $0x80000001, %eax			# enable PG, and enters LM
	movl %eax, %cr0				# compatibility submode

	ljmpl *_ljmp64_ptr-_base(%ebx)		# long jump to 64-bit
	
	.code64
	.align 8, 0x90
_start_64:
	xorl %eax, %eax				# nullify %ds, %ss
	mov %eax, %ds
	mov %eax, %ss

	and %ebx, %ebx				# clear upper bits of %ebx

	mov %ebx, %edi				# cpuid will clobber %ebx
	mov $1, %eax				# A = 1
	xor %ecx, %ecx				# C = 0
	cpuid
	shr $24, %ebx				# apic id
	xchg %edi, %ebx

	mov _per_cpu_rsp-_base(%rbx, %rdi, 8), %rsp	# Initialize stack	
	movq _entry-_base(%rbx), %rax			# hypervisor smp entry point
	call *%rax
1:	jmp 1b
	
	.align 4
_ljmp32_ptr:
	.long _start_32-_base			# %eip, needs fixing
	.short 0x10				# %cs

	.align 4
_ljmp64_ptr:
	.long _start_64-_base			# %eip, needs fixing
	.short 0x8				# %cs

	.align 4
	.short 0				# align gdt_ptr to 4b + 2
_gdt_ptr:
	.short 0x3f				# limit
	.long  _gdt-_base			# linear addr, needs fixing

	.align 4
_cr3_32:
	.long 0
_cr3_64:
	.long 0
_ia32_efer:
	.long 0
	
	.align 8
_entry:
	.quad 0					# entry point

	.align 8
_gdt:
	.space 0x40				# needs fixing, obviously

	.align 8
_per_cpu_rsp:
	.space 0x40

_end:


# Export these absolute addresses so C code can copy trampoline template out
	.global trampoline_template_start
trampoline_template_start = _base
	.global trampoline_template_end
trampoline_template_end   = _end

	.macro EXPORT_OFFSET name
	.align 4
	.global	trampoline_offset\name
trampoline_offset\name:
	.long	\name-_base
	.endm
	
# Export these offsets so C code can fill these values in after copying out
EXPORT_OFFSET	_gdt
EXPORT_OFFSET	_per_cpu_rsp
EXPORT_OFFSET	_cr3_32
EXPORT_OFFSET	_cr3_64
EXPORT_OFFSET	_ia32_efer
EXPORT_OFFSET	_entry

# Export a offset table so C code can adjust these values after copying out
	.global trampoline_relocation_offsets
	.align 4
trampoline_relocation_offsets:	
	.long _ljmp32_ptr-_base
	.long _ljmp64_ptr-_base
	.long _gdt_ptr+2-_base
	.long 0
