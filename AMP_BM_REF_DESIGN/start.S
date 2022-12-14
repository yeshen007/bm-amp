/*
 *
 * Copyright (C) 2014 Embest Technology Co., Ltd. <http://www.embest-tech.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  - Neither the name of Embest Technology Co., Ltd. nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS", 
 * it is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

	 
	.globl start
	.globl ledblink
	.globl ledblink1

	.globl usr_stack_end
	

.equ    PRRR,  0xff0a81a8
.equ    NMRR,  0x40e040e0

#define Mode_USR 0x10
#define Mode_FIQ 0x11
#define Mode_IRQ 0x12
#define Mode_SVC 0x13
#define Mode_ABT 0x17
#define Mode_UND 0x1B
#define Mode_SYS 0x1F

#define I_Bit    0x80 
#define F_Bit    0x40 

#define TTB_S           (1 << 1)
#define TTB_RGN_NC      (0 << 3)
#define TTB_RGN_OC_WBWA (1 << 3)
#define TTB_RGN_OC_WT   (2 << 3)
#define TTB_RGN_OC_WB   (3 << 3)
#define TTB_NOS         (1 << 5)
#define TTB_IRGN_NC     ((0 << 0) | (0 << 6))
#define TTB_IRGN_WBWA   ((0 << 0) | (1 << 6))
#define TTB_IRGN_WT     ((1 << 0) | (0 << 6))
#define TTB_IRGN_WB     ((1 << 0) | (1 << 6))

/* PTWs cacheable, inner WB not shareable, outer WB not shareable */
#define TTB_FLAGS_UP    TTB_IRGN_WB|TTB_RGN_OC_WB
#define PMD_FLAGS_UP    PMD_SECT_WB

/* PTWs cacheable, inner WBWA shareable, outer WBWA not shareable */
#define TTB_FLAGS_SMP   TTB_IRGN_WBWA|TTB_S|TTB_NOS|TTB_RGN_OC_WBWA
#define PMD_FLAGS_SMP   PMD_SECT_WBWA|PMD_SECT_S

#define CONFIG_OSC_TIMER1_BASE 			0xffd01000
#define CONFIG_OSC_TIMER_CUR_CNT				0x4
		
start:	
        b	reset
        LDR     PC, Undefined_Addr
        LDR     PC, SVC_Addr	
        LDR     PC, Prefetch_Addr
        LDR     PC, Abort_Addr
        NOP     @ Reserved for secure montor calls
        LDR     PC, IRQ_Addr
        LDR     PC, FIQ_Addr
        
        
Undefined_Addr:	 .word     CPU_ARM_ExceptUndefInstrHndlr
SVC_Addr:        .word     CPU_ARM_ExceptSwiHndlr
Prefetch_Addr:   .word     CPU_ARM_ExceptPrefetchAbortHndlr
Abort_Addr:      .word     CPU_ARM_ExceptDataAbortHndlr
IRQ_Addr:        .word     CPU_ARM_ExceptIrqHndlr
FIQ_Addr:        .word     CPU_ARM_ExceptFiqHndlr


reset:

	/* ?????????svc?????????????????????IRQ???FIQ?????? */
	mrs	r0, cpsr			
	bic	r0, r0, #0x1f
	orr	r0, r0, #0xd3
	msr	cpsr_cxsf,r0	

	
	/* ??????L1 i-cache???L1 d-cache,?????????????????? */
    mov     r0, #0					@ r0 = 0x00000000
 	mcr     p15, 0, r0, c7, c5, 0   @ ?????????I-cache
    mcr     p15, 2, r0, c0, c0, 0	@ ???0x00000000??????CSSELR(Cache Size Selection Register)
    								@ ???????????????????????????L1 cache	
    								
    mrc     p15, 1, r0, c0, c0, 0	@ ???CCSIDR(Cache Size ID Registers)??????r0
    ldr     r1, =0x7fff				@ r1 = 0x7fff
    and     r2, r1, r0, lsr #13		@ r2 = r1 & (r0 >> 13) 
									@ ???CCSIDR???13??????????????????????????????13???????????????????????????????????????NumSets
    ldr     r1, =0x3ff				@ r1 = 0x3fff
    and     r3, r1, r0, lsr #3      @ r3 = r1 & (r0 >> 3)   (Associativity of cache(NumWays) - 1)
    								@ ???CCSIDR???3??????????????????????????????13??????????????????19??????????????????Associativity
    add     r2, r2, #1              @ NumSets
   
    and     r0, r0, #0x7	@ r0 = r0 & 7,?????????CCSIDR????????????(Log2(Number of words in cache line)) -2
    add     r0, r0, #4      @ SetShift

    clz     r1, r3          @ WayShift   
    add     r4, r3, #1      @ NumWays
1:      
	sub     r2, r2, #1      @ NumSets--
    mov     r3, r4          @ Temp = NumWays
2:     
	subs    r3, r3, #1      @ Temp--
    mov     r5, r3, lsl r1
    mov     r6, r2, lsl r0
    orr     r5, r5, r6      @ Reg = (Temp<<WayShift)|(NumSets<<SetShift)
    mcr     p15, 0, r5, c7, c6, 2
    bgt     2b
    cmp     r2, #0
    bgt     1b
    dsb
    isb


	/* ???????????????????????????0x00000000 */
	mrc     p15, 0, r0, c1, c0, 0   @ SCTLR??????r0
    bic     r0, #(0x1<<13)    		@ V = 0
    mcr     p15, 0, r0, c1, c0, 0   @ Write CP15 SCTRL Register


    @@ ???Monitor mode???Hyp mode??????????????????????????????????????????start??? 
    ldr     r0, =start  
    mcr     p15, 0, r0, c12, c0, 0  @ Set VBAR

	@@ neon???vfp??????
    mrc     p15, 0, r0, c1, c0, 2	@ CPACR??????r0	
    orr     r0, r0, #(0x3  <<20)	@ ??????r0??????20???21???(??????pl????????????cp10)
    orr     r0, r0, #(0x3  <<22)	@ ??????r0??????22???23???(??????pl????????????cp11)	
    mcr     p15, 0, r0, c1, c0, 2	@ ??????CPACR
    isb								@ ????????????
    mov     r0, #(0x1 << 30)  		
	@vmsr	fpexc, r0 <- not correctly supported in binutils 2.22
	mcr     p10, 7, r0, cr8, cr0, 0 @ alternate mnemonic for vmsr fpexec, r0
 
	@@ ?????????
	ldr sp, =usr_stack_end


	//??????????????????,mmu?????????main?????????
	bl  build_PGTL

	@@ Set TTBCR to use N=0

	@@ ????????????????????????,See ARMv7-A,R, section B4.1.154
	orr    r0, r0, #TTB_FLAGS_SMP		@ ??????TTBR0??????1???3???5???6???
	mcr    p15, 0, r0, c2, c0, 0		

	isb
	dsb

	@@ ???????????????????????????????????????????????? 
	ldr    r0, =0xFFFFFFFF             @ Set full access for all domains
	mcr    p15, 0, r0, c3, c0, 0		

	@@ ??????smp,??????armv7???????????????a9?????????
	mov r0, #0x41
	mcr	p15, 0, r0, c1, c0, 1
	isb
	dsb

	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ ??????icache
	dsb
	mcr	p15, 0, r0, c8, c7, 0       @ ??????TLBs
	

	dsb
	isb

	
jump_to_virt:	

	ldr sp, =usr_stack_end

	@@ set the stack pinters for svc and irq mode. We don@t do anything
	@@ that needs a stack in the other modes so do not bother 
	@@ setting them up
	msr     CPSR_c, #Mode_IRQ | I_Bit | F_Bit		//?????????IRQ????????????irq?????????fiq??????
	ldr	sp,=irq_stack_end							//??????IRQ??????????????????

    msr     CPSR_c, #Mode_SVC | I_Bit | F_Bit		//?????????SVC????????????irq?????????fiq??????

	// ???bss 
	ldr	r0,=bss_start		@ find start of bss segment
	ldr	r1,=bss_end			@ stop here
	mov	r2, #0x00000000		@ clear value
clbss_l:	
	str	r2, [r0]		@ clear BSS location
	cmp	r0, r1			@ are we at the end yet
	add	r0, r0, #4		@ increment clear index pointer
	bne	clbss_l			@ keep clearing till at end

	@b ledblink

	ldr	pc, =main	//?????????main



delay:
	push {r5}
	@mov r5, #0x4000000
	mov r5, #0xf000
1:	subs r5,r5,#1
	cmp r5,#0
	bne 1b
	pop {r5}
	mov pc,lr


	.align
UART1BASE:
        .word 0xFF903000

GPIO5_BASE_ADDR:
        .word 0xff709000

GPIO5_DIR_ADDR:
        .word 0xff709004

on_led:
	ldr r2,GPIO5_DIR_ADDR
	ldr r1,[r2, #0]
	orr r1, r1, #(0xf<<12)		
	str r1,[r2]
	ldr r2,GPIO5_BASE_ADDR
	ldr r1,[r2, #0]
	bic r1, r1, #(0xf<<12)
	str r1,[r2]
	mov pc, lr


off_led:
	ldr r2,GPIO5_BASE_ADDR
	ldr r1,[r2, #0]
	orr r1, r1, #(0xf<<12)
	str r1,[r2]
	mov pc, lr


ledblink:
	bl on_led
	bl delay
	bl off_led
	bl delay
	b ledblink

ledblink1:
	push {lr}
	bl on_led
	bl delay
	bl off_led
	bl delay
	pop {lr}
	mov pc, lr

/*
***************************************************************************
*						Embest Tech co., ltd
*						 www.embest-tech.com
***************************************************************************
*
*this small code is for loading the bare metal from the 0x0 address.
*
*/


	@@__CPUINIT
	.arch	armv7-a


	.globl secondary_trampoline
	.align 4
secondary_trampoline:
		movw	r2, #:lower16:0xffd080c4
		movt  r2, #:upper16:0xffd080c4

		ldr 	r0, [r2]
		bx r0
		@@ldr	  r1, [r0]
		@@bx	  r1


	.globl secondary_trampoline_end
	.align 4
secondary_trampoline_end:




	.data 
	.align 5
fiq_stack_start:	
	.space 1024
fiq_stack_end:	
	.space 4096
irq_stack_end:	
	.space 16384
usr_stack_end:
	.space 4096
usr_heap_end:

 
