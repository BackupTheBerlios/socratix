/*
 *  Authors:	Benedikt Meurer <bmeurer@fwdn.de>
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
 */


#define KERNEL_CS	0x08
#define KERNEL_DS	0x10

#define FKERNEL_SEG	0x0000	/* final kernel segment */
#define TKERNEL_SEG	0x1000	/* temp. kernel segment */


.code16
.text
.data
.bss
.text

.global _start
_start:
	/* load ds, es with cs */
	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %es

	/* tell user, we're entering setup */
	movw	$entering_msg, %si
	call	puts

	/* now, detect the users cpu */
	call	detect_cpu

	/* save the BIOS Info Block to 0x9F000 */
	call	save_bios_inf_blk

	/* move kernel image to final destination */
	call	move_kernel


/*
 * ok, that was easy, now switch to protected mode
 */
	/* mask all irqs */
	call	mask_irqs

	/* load gdt */
	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %es
	xorw	%di, %di
	lgdt	gdt_description


	/* enable PE flag */
	movl	%cr0, %eax
	orb	$0x01, %al
	movl	%eax, %cr0

	jmp	entered_pmode

entered_pmode:
	.byte	0xea
	.word	(FKERNEL_SEG * 16)
	.word	KERNEL_CS


/*
 * puts a 0-terminated message to screen
 */
puts:	lodsb
	orb	%al, %al
	jz	done
	movb	$0x0E, %ah
	movw	$7, %bx
	int	$0x10
	jmp	puts
done:	ret


/*
 * This routine saves the BIOS Information Block
 * to 0x9F000
 */
save_bios_inf_blk:
	cld
	movw	$0x0000, %ax
	movw	%ax, %ds
	movw	$0x9F00, %ax
	movw	%ax, %es
	movw	$4096, %cx

	xorw	%di, %di
	xorw	%si, %si
	rep	movsb
	ret


/*
 * This routine loads the kernel image to its
 * final destination from TKERNEL_SEG to 
 * FKERNEL_SEG (0x00000), for now, we're limited
 * to a max. kernel size of 256 KB.
 */
move_kernel:
	cld

	movw	$0x9000, %ax	/* get the kernel size from 0x9000:508 */
	movw	%ax, %fs	/* of the bootsect */
	movw	%fs:508, %cx
	movw	%cx, %dx
	shlw	$2, %cx		/* cause dwords = (kernel_size * 16) / 4 */

	/* load data segments: ds = temp. kernel, es = final kernel pos */
	movw	$TKERNEL_SEG, %ax
	movw	%ax, %ds
	movw	$FKERNEL_SEG, %ax
	movw	%ax, %es

	xorw	%di, %di
	xorw	%si, %si
	rep	movsl

	ret


/*
 * This routine masks all maskable interrupts
 */
mask_irqs:
	movb	$0xFF, %al
	outb	%al, $0xA1
	outb	%al,$0x80	/* delay */
	movb	$0xFB, %al	/* IRQ2 is cascaded */
	outb	%al, $0x21
	cli
	ret


/*
 * detect the installed cpu:
 *  Check if the user has installed a 8088/8086, a 80286 or a i386 or higher.
 */
detect_cpu:
	movw	$detecting_msg, %si	/* tell user, what we're doing */
	call	puts

	/* test for 8088/8086 -> flag bits 12-15 will be set */
	pushf
	xorb	%ah, %ah
	pushw	%ax
	popf
	pushf
	popw	%ax
	andb	$0x0F, %ah	/* check if bits 12-15 are set */
	cmpb	$0x0F, %ah
	je	_dc_8088

	/* check for a 80286 -> flag bits 12-15 are clear */
	movb	$0xF0, %ah
	pushw	%ax
	popf
	pushf
	popw	%ax
	andb	$0xF0, %ah
	jz	_dc_80286

	/* ok, we got a i386+ */
	popf
	movw	$i386_msg, %si
	call	puts
	ret

_dc_8088:
	movw	$i8088_msg, %si
	jmp	_dc_need_i386

_dc_80286:
	movw	$i80286_msg, %si

_dc_need_i386:
	call	puts
	movw	$need_i386_msg, %si
	call	puts

	/* now we go to an endless loop */
_dcdie:	jmp	_dcdie


/*
 * Notification messages to the user
 */
entering_msg:
	.asciz	"\n\rOk, now entering the setup...\n\r"

detecting_msg:
	.asciz	"detecting cpu... "

i8088_msg:
	.asciz	"found 8088/8086\n\r"

i80286_msg:
	.asciz	"found 80286\n\r"

i386_msg:
	.asciz	"found i386+\n\r"

need_i386_msg:
	.asciz	"Sorry, but you need to have at least a i386 cpu installed\n\r"

done_msg:
	.asciz	"done\n\r"


#if 0
/*
 * temporary gdt
 */
.align	2
gdt:
	.quad	0x0000000000000000	/* null */
	.quad	0x00CF9A000000FFFF	/* code (KERNEL_CS) */
	.quad	0x00CF92000000FFFF	/* data (KERNEL_DS) */
	.quad	0x0000000000000000	/* temp */
#endif


.word	0
gdt_description:
	.word	(4 * 8) -1		/* 4 entries */
	.long	0x0010			/* gdt in kernel image */


