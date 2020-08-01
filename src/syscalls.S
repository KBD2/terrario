.text

.global _SMEM_optimize_frame
.global _SMEM_optimize_codelength

.align 4

_SMEM_optimize_frame:

        mov.l   r14, @-r15
        mov.l   r13, @-r15
        sts.l   pr, @-r15
        add     #-4, r15
        mov     r4, r13

		mov.l 	.SMEM_Optimization, r0
		mov.l	.syscall_table, r2
		jsr 	@r2
		nop

		mov.l 	.App_RegisterAddins, r0
		mov.l	.syscall_table, r2
		jsr		@r2
        nop

        mov     #0, r14
.try:
        mov.l .nameOffset, r5
        mov r15, r6

		mov.l 	.GetAddinHeaderAddr, r0
		mov.l	.syscall_table, r2
		jsr		@r2
        mov r14, r4
       
        tst     r0, r0
        bt/s    .quit
        mov     #-1, r0
    
        mov.l @r15, r4
        mov #8, r6

		mov.l 	.strncpm, r0
		mov.l	.syscall_table, r2
		jsr		@r2
        mov r13, r5

        tst     r0, r0
        bt      .exit
        bra     .try
        add     #1, r14
       
.exit:   
		mov.l 	.App_BuiltInCount, r0
		mov.l	.syscall_table, r2
		jsr		@r2
        nop

        add     r0, r14
       
        mov.l .App_Start, r0
        mov.l .syscall_table, r2
        mov     #0, r4
        mov     #0, r5
        mov     #1, r7
        mov     r14, r6
               
        add     #4, r15
        lds.l   @r15+, pr
        mov.l   @r15+, r13
        jmp @r2
        mov.l   @r15+, r14
               
.quit:
        add     #4, r15
        lds.l   @r15+, pr
        mov.l   @r15+, r13
        rts
        mov.l   @r15+, r14
       
        .align 4

_SMEM_optimize_codelength:     
        .long (_SMEM_optimize_codelength-_SMEM_optimize_frame )

.syscall_table: .long	0x80010070

.nameOffset: .long 0x1D4

.App_Start: 			.long 0x049A
.SMEM_Optimization:		.long 0x019F
.App_RegisterAddins:	.long 0x0005
.GetAddinHeaderAddr:	.long 0x000A
.strncpm:				.long 0x0AD8
.App_BuiltInCount:		.long 0x046B