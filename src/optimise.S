.global _SMEM_optimise_frame
.global _SMEM_optimise_codelength
.global _App_GetAddinEstripInfo

.section .rodata

//; int SMEM_optimise_frame (unsigned char *title);
_SMEM_optimise_frame:
		mov.l	r14, @-r15
		mov.l	r13, @-r15
		sts.l	pr, @-r15
		add		#-4, r15
		mov		r4, r13

		bsr		_SMEM_Optimisation
		nop

//; Re-register the installed addins
		bsr		_App_RegisterAddins
		nop

//; Find the addin's index
		mov		#0, r14
try:
		mov.l	addinHeaderOffset, r5
		mov		r15, r6
		bsr		_GetAddinHeaderAddr
		mov		r14, r4

		tst		r0, r0
		bt/s	quit
		mov		#-1, r0

		mov.l	@r15, r4
		mov		#8, r6
		bsr		_strncmp
		mov		r13, r5

		tst		r0, r0
		bt		exit
		bra		try
		add		#1, r14


//; Start the addin
exit:
		bsr _App_BuiltInCount
		nop
		add		r0, r14

		mov.l App_Start, r0
		mov.l syscallTable, r2

		mov		#0, r4
		mov		#1, r5
		mov		r5, r7
		mov		r14, r6

		add		#4, r15
		lds.l	@r15+, pr
		mov.l	@r15+, r13
		jmp @r2
		mov.l	@r15+, r14

//; Addin title didn't exist
quit:
		add		#4, r15
		lds.l	@r15+, pr
		mov.l	@r15+, r13
		rts
		mov.l	@r15+, r14

#define syscall(code) \
	mov.l syscallTable, r2 ;\
	mov.w 1f, r0 ;\
	jmp @r2 ;\
	nop ;\
1: .word code

_SMEM_Optimisation:
	syscall(0x019F)
_App_RegisterAddins:
	syscall(0x0005)
_GetAddinHeaderAddr:
	syscall(0x000A)
_strncmp:
	syscall(0x0AD8)
_App_BuiltInCount:
	syscall(0x046B)
_App_GetAddinEstripInfo:
	syscall(0x000E)

.align 4

App_Start:
	.long 0x049A

addinHeaderOffset:
	.long 0x1D4

syscallTable:
	.long 0x80010070

_SMEM_optimise_codelength:
	.long (_SMEM_optimise_codelength - _SMEM_optimise_frame)