/*
 *  Authors:	Benedikt Meurer <bmeurer@fwdn.de>
 *		Volker Stroebel <mmv1@linux4us.de>
 *
 *  Copyright (c) 2001 Socratix Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 * ---------
 *
 * This is the head of the kernel. It must be always the first file to
 * link from the kernel files. It includes the 32bit startup code.
 *
 * All Code/Data within the first page (4K) will be lost after paging
 * init, cause the first page will be not present, to prevent NULL
 * pointers within the kernel. So don't store anything within the
 * first page, that you'll need later again.
 *
 */


#include <asm/page.h>
#include <asm/segment.h>


.text

.global _start
_start:
	jmp	_start32


/*
 * Temp. Global Descriptor Table (GDT), needed by setup.S
 */
.org	0x0010
.global	gdt, gdt_description
tmp_gdt:
	.quad	0x0000000000000000	/* null */
	.quad	0x00CF9A000000FFFF	/* code (KERNEL_CS) */
	.quad	0x00CF92000000FFFF	/* data (KERNEL_DS) */
	.quad	0x0000000000000000	/* temp */


/*
 * This routine initializes the IDT with
 * the dummy_irq handler
 */
.align 2
.global	init_idt
init_idt:
	lea	dummy_irq, %edx
	movl	$(KERNEL_CS << 16), %eax
	movw	%dx, %ax		/* a little bit tricky */
	movw	$0x8E00, %dx		/* irq gate: DPL=0 | Present */
	lea	idt, %edi
	movl	$256, %ecx		/* 256 entries in the idt */
_idt_init_rep:
	movl	%eax, (%edi)
	movl	%edx, 4(%edi)
	addl	$8, %edi
	dec	%ecx
	jne	_idt_init_rep
	lidt	idt_description		/* load the idt description */
	ret				/* ok, we got it, return */


/*
 * Do kernel startup
 */
.align	2
_start32:
	/* init data and stack selectors */
	movl	$KERNEL_DS, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss


	/* Clear BSS first so that there'll be no surprises... */
	cld
	xorl	%eax, %eax
	movl	$__bss_start, %edi
	movl	$_end, %ecx
	subl	%edi, %ecx
	rep	stosb


	/* load ss:esp with values in kstack_description */
	lss	kstack_description, %esp


	/* init the idt with the dummy_irq, but let irq's disabled */
	call	init_idt
	lgdt	gdt_description


	/*
	 * NOTE!!
	 * start_kernel shouldn't return. If start_kernel tries to
	 * return, there'll be a page fault, cause after paging init
	 * the first page is no longer present, to prevent NULL
	 * pointers within the kernel
	 */
	cld
	xorl	%eax, %eax
	call	start_kernel


/*
 * the kernel page directory
 *
 * NOTE: kernel page tables gets allocated dynamically by init_paging
 * directly after the kernel heap.
 */
.org	0x1000
.global	pg_dir
pg_dir:
	.fill	PAGE_SIZE, 1, 0x0


.org	0x2000


/*
 * The global Descriptor Table
 */
.align	2
.global	gdt, gdt_description
gdt:
	.quad	0x0000000000000000	/* null */
	.quad	0x00CF9A000000FFFF	/* code (KERNEL_CS) */
	.quad	0x00CF92000000FFFF	/* data (KERNEL_DS) */
	.quad	0x0000000000000000	/* temp */

.word	0
gdt_description:
	.word	(4 * 8) - 1		/* contains 4 entries */
	.long	gdt


/*
 * Interrupt Descriptor Table (IDT)
 */
.align	2
.global	idt, idt_description
idt:
	.fill	256, 8, 0x0

.word	0
idt_description:
	.word	(256 * 8) - 1		/* contains 256 entries */
	.long	idt


/*
 * the default interrupt "handler"
 * written by Volker. Seems to work good,
 * after a few improvements.
 */
.align	2
.global	dummy_irq
dummy_irq:	
	cld
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushw	%es
	pushw	%ds
	movl	$KERNEL_DS, %eax
	movw	%ax, %ds
	movw	%ax, %es
#if 0
	pushl	$dummyirq_msg
	call	printk
	popl	%eax

	movb	$0x20, %al
	outb	%al, $0xa0
	outb	%al, $0x20
#endif
	popw	%ds
	popw	%es
	popl	%edx
	popl	%ecx
	popl	%eax
	iret


dummyirq_msg:
	.asciz	"Unhandled IRQ\n"

