.macro PUSH_ALL_REGS
	push	%r15
	push	%r14
	push	%r13
	push	%r12
	push	%r11
	push	%r10
	push	%r9
	push	%r8
	push	%rdi
	push	%rsi
	push	%rbp
	subq	$1*8, %rsp		# leave room for rsp
	push	%rbx
	push	%rdx
	push	%rcx
	push	%rax
.endm

.macro POP_ALL_REGS
	pop	%rax
	pop	%rcx
	pop	%rdx
	pop	%rbx
	pop	%rbp			# skip rsp slot
	pop	%rbp
	pop	%rsi
	pop	%rdi
	pop	%r8
	pop	%r9
	pop	%r10
	pop	%r11
	pop	%r12
	pop	%r13
	pop	%r14
	pop	%r15
.endm

.macro POP_ALL_SVM_REGS
	pop	%rcx			# skip rax slot
	pop	%rcx
	pop	%rdx
	pop	%rbx
	pop	%rbp			# skip rsp slot
	pop	%rbp
	pop	%rsi
	pop	%rdi
	pop	%r8
	pop	%r9
	pop	%r10
	pop	%r11
	pop	%r12
	pop	%r13
	pop	%r14
	pop	%r15
.endm

ENTRY exception_common
	# err code and vector number have already been pushed
	PUSH_ALL_REGS

	mov	%rsp, %rdi
	call	exception

	POP_ALL_REGS

	addq	$2*8, %rsp	# skip vector number and error code
	iretq

.macro exception num sym errcode=0
ENTRY \sym
.if (\errcode == 0)
	push	$0
.endif
	push	$\num
	jmp	exception_common
.endm

.macro intvec num
exception \num intvec\num
.endm

exception 0 div_zero
exception 1 debug
exception 2 nmi
exception 3 breakpoint
exception 4 overflow
exception 5 bounds
exception 6 invalid_opcode
exception 7 no_device
exception 8 double_fault 1
exception 9 exception9
exception 10 invalid_tss 1
exception 11 seg_not_present 1
exception 12 stack_fault 1
exception 13 gen_protect 1
exception 14 page_fault 1
exception 15 exception15
exception 16 fp_error
exception 17 align_check 1
exception 18 machine_check
exception 19 simd_error

.data
.global intvec_start
intvec_start:
	.quad intvec20
.global intvec_len
intvec_len:
	.quad intvec33 - intvec32

.text

intvec 20
intvec 21
intvec 22
intvec 23
intvec 24
intvec 25
intvec 26
intvec 27
intvec 28
intvec 29
intvec 30
intvec 31
intvec 32
intvec 33
intvec 34
intvec 35
intvec 36
intvec 37
intvec 38
intvec 39
intvec 40
intvec 41
intvec 42
intvec 43
intvec 44
intvec 45
intvec 46
intvec 47
intvec 48
intvec 49
intvec 50
intvec 51
intvec 52
intvec 53
intvec 54
intvec 55
intvec 56
intvec 57
intvec 58
intvec 59
intvec 60
intvec 61
intvec 62
intvec 63
intvec 64
intvec 65
intvec 66
intvec 67
intvec 68
intvec 69
intvec 70
intvec 71
intvec 72
intvec 73
intvec 74
intvec 75
intvec 76
intvec 77
intvec 78
intvec 79
intvec 80
intvec 81
intvec 82
intvec 83
intvec 84
intvec 85
intvec 86
intvec 87
intvec 88
intvec 89
intvec 90
intvec 91
intvec 92
intvec 93
intvec 94
intvec 95
intvec 96
intvec 97
intvec 98
intvec 99
intvec 100
intvec 101
intvec 102
intvec 103
intvec 104
intvec 105
intvec 106
intvec 107
intvec 108
intvec 109
intvec 110
intvec 111
intvec 112
intvec 113
intvec 114
intvec 115
intvec 116
intvec 117
intvec 118
intvec 119
intvec 120
intvec 121
intvec 122
intvec 123
intvec 124
intvec 125
intvec 126
intvec 127
intvec 128
intvec 129
intvec 130
intvec 131
intvec 132
intvec 133
intvec 134
intvec 135
intvec 136
intvec 137
intvec 138
intvec 139
intvec 140
intvec 141
intvec 142
intvec 143
intvec 144
intvec 145
intvec 146
intvec 147
intvec 148
intvec 149
intvec 150
intvec 151
intvec 152
intvec 153
intvec 154
intvec 155
intvec 156
intvec 157
intvec 158
intvec 159
intvec 160
intvec 161
intvec 162
intvec 163
intvec 164
intvec 165
intvec 166
intvec 167
intvec 168
intvec 169
intvec 170
intvec 171
intvec 172
intvec 173
intvec 174
intvec 175
intvec 176
intvec 177
intvec 178
intvec 179
intvec 180
intvec 181
intvec 182
intvec 183
intvec 184
intvec 185
intvec 186
intvec 187
intvec 188
intvec 189
intvec 190
intvec 191
intvec 192
intvec 193
intvec 194
intvec 195
intvec 196
intvec 197
intvec 198
intvec 199
intvec 200
intvec 201
intvec 202
intvec 203
intvec 204
intvec 205
intvec 206
intvec 207
intvec 208
intvec 209
intvec 210
intvec 211
intvec 212
intvec 213
intvec 214
intvec 215
intvec 216
intvec 217
intvec 218
intvec 219
intvec 220
intvec 221
intvec 222
intvec 223
intvec 224
intvec 225
intvec 226
intvec 227
intvec 228
intvec 229
intvec 230
intvec 231
intvec 232
intvec 233
intvec 234
intvec 235
intvec 236
intvec 237
intvec 238
intvec 239
intvec 240
intvec 241
intvec 242
intvec 243
intvec 244
intvec 245
intvec 246
intvec 247
intvec 248
intvec 249
intvec 250
intvec 251
intvec 252
intvec 253
intvec 254
intvec 255
