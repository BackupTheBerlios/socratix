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


#define BOOT_SIZE	256
#define BOOT_SEG	0x07C0

#define	INIT_SEG	0x9000
#define	STACK_END	0xEFFC

/* setup gets loaded directly after the bootsect (512 = 0x200) */
#define SETUP_ADDR	0x0200	/* addr of setup in INIT_SEG */
#define	SETUP_SEG	0x9020	/* segment for setup */

#define	KERNEL_SEG	0x1000


.code16
.text

.global _start
_start:
	jmp	load

/*
 * move us away from $BOOT_SEG to $INIT_SEG and jump there
 */
load:	movw	$BOOT_SEG, %ax
	movw	%ax, %ds
	movw	$INIT_SEG, %ax
	movw	%ax, %es
	movw	$BOOT_SIZE, %cx
	xorw	%si, %si
	xorw	%di, %di
	rep	movsw
	ljmp	$INIT_SEG, $main


main:	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %es

	/* place stack */
	cli
	movw	$STACK_END, %ax
	movw	%ax, %sp
	movw	$INIT_SEG, %ax
	movw	%ax, %ss
	sti

	pusha

	/* try to enable a20 */
	call	enable_a20

	/* print loading message */
	movw	$setup_msg, %si
	call	puts

	/* reset floppy */
	call 	reset_floppy

	popa

	/* load setup - int 0x13/0x02 */
	movw	$SETUP_ADDR, %bx
	movb	$0x02, %ah
	movb	setup_sects, %al
	movw	$0x0002, %cx
	xorw	%dx, %dx
	int	$0x13

	/* hopefully, setup is loaded now */
	movw	$done_msg, %si
	call	puts

	/* put kernel loading message */
	movw	$kernel_msg, %si
	call	puts

	/* load the kernel */
	call	read_kernel

	/* kill floppy motor */
	call	stop_motor

	/* ok, we got it :) */
	movw	$done_msg, %si
	call	puts

	/* jump to setup */
	ljmp	$SETUP_SEG, $0 


/*
 * Ok, now we can start, reading the system kernel from disk
 * to 0x10000.
 */
sread:	.word	0x0	/* number of sectors read from this track */
head:	.word	0x0	/* the currently acting head */
track:	.word	0x0	/* current track */

read_kernel:
	movw	$KERNEL_SEG, %ax
	movw	%ax, %es
	xorw	%bx, %bx	/* we start at 0x1000:0x0 */
	movb	setup_sects, %al
	incb	%al
	movb	%al, sread

read_rep:
	/* test, if loading of kernel complete */
	movw	%es, %ax
	subw	$KERNEL_SEG, %ax
	cmpw	kernel_size, %ax
	jbe	_read_k1

	/* ok, we got it :) */
	ret

_read_k1:
	movw	$18, %ax		/* we assume, 18 sectors */
	subw	sread, %ax
	movw	%ax, %cx
	shlw	$9, %cx
	addw	%bx, %cx
	jnc	_read_k2
	
	je	_read_k2

	xorw	%ax, %ax
	subw	%bx, %ax
	shrw	$9, %ax

_read_k2:
	call	read_track	/* read the track */
	movw	%ax, %cx
	addw	sread, %ax
	cmpw	$18, %ax		/* we assume 18 sectors */
	jne	_read_k4
	
	movw	$1, %ax
	subw	head, %ax
	jne	_read_k3
	
	incw	track
_read_k3:
	movw	%ax, head
	xorw	%ax, %ax
_read_k4:
	movw	%ax, sread
	shlw	$9, %cx
	addw	%cx, %bx
	jnc	read_rep
	
	movw	%es, %ax
	addb	$0x10, %ah
	movw	%ax, %es
	xorw	%bx, %bx
	jmp	read_rep

read_track:
	pusha

	/* print dot */
	pusha
	movw	$dot_msg, %si
	call	puts
	popa

	movw	track, %dx
	movw	sread, %cx
	incw	%cx
	movb	%dl, %ch
	movw	head, %dx
	movb	%dl, %dh
	andw	$0x0100, %dx
	movb	$2, %ah
	int	$0x13
	jc	track_error
	popa
	ret

track_error:
	movw	$error_msg, %si
	call	puts
die:	jmp	die


/*
 * This routine enables A20. If A20 couldn't be enabled, it
 * goes to an infinite loop.
 */
enable_a20:
	pushw	%ds
	pushw	%es

	call	kbd_empty
	movb	$0xD1, %al
	outb	%al, $0x64
	call	kbd_empty

	movb	$0xDF, %al
	outb	%al, $0x60
	call	kbd_empty

	/* enable "fast A20", cause some chips have only this */
	inb	$0x92, %al
	orb	$0x02, %al
	outb	%al, $0x92
	call	kbd_empty

	/* wait until A20 is really enabled */
	xorw	%ax, %ax			/* segment 0x0000 */
	movw	%ax, %fs
	decw	%ax				/* segment 0xffff (HMA) */
	movw	%ax, %gs
a20_wait:
	incw	%ax				/* unused memory location <0xfff0 */
	movw	%ax, %fs:(0x200)		/* we use the "int 0x80" vector */
	cmpw	%gs:(0x210), %ax		/* and its corresponding HMA addr */
	je	a20_wait			/* wait until no longer aliased */

	popw	%es
	popw	%ds

	ret


/*
 * This routine checks that the keyboard command queue is empty
 * No timeout is used - if this hangs there is something wrong with
 * the machine, and we probably couldn't proceed anyway
 */
kbd_empty:
	inb	$0x0064, %al	/* 8042 status port */
	test	$2, %al		/* is input buffer full? */
	jnz	kbd_empty	/* yes -> loop */
	ret


/* puts a 0-terminated message to screen */
puts:	lodsb
	orb	%al, %al
	jz	done
	movb	$0x0E, %ah
	movw	$7, %bx
	int	$0x10
	jmp	puts
done:	ret


/*
 * reset the floppy disk controller
 */
reset_floppy:
	xorb	%ah, %ah
	xorb	%dl, %dl
	int 	$0x13
	ret


/*
 * stop the floppy motor
 */
stop_motor:
	movw	$0x03F2, %dx
	xorb	%al, %al
	outb	%al, %dx
	ret


setup_msg:
	.ascii	"loading setup...\0"

kernel_msg:
	.ascii	"loading kernel"

dot_msg:
	.ascii	".\0"

done_msg:
	.ascii	" done\n\r\0"

error_msg:
	.ascii	"error while loading kernel image\n\r\0"


.org	507

setup_sects:	.byte	0x0	/* sectors of the setup */
kernel_size:	.word	0x0	/* size of the kernel (in byte / 16) */

.word	0xAA55

