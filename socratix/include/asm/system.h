/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 *  Authors:	Benedikt Meurer <bmeurer@fwdn.de>
 *
 *  Copyright (c) 2001 Socratix Development Team
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
 * System macros, definitions, utilities
 *
 */


#ifndef __ASM_SYSTEM_H
#define __ASM_SYSTEM_H


/* define NULL */
#ifndef NULL
#define NULL	((void *) 0L)
#endif


/* enable/disable blockable interrupts */
#define cli()	{ asm volatile ("cli" ::: "memory" ); }
#define sti()	{ asm volatile ("sti" ::: "memory" ); }


/* no op, halt */
#define nop()	{ asm volatile ("nop" ::: "memory" ); }
#define hlt()	{ asm volatile ("hlt" ::: "memory" ); }


/* disable all masable interrupts */
#define disable_irqs() \
{ \
	asm volatile ("outb %%al, $0xA1; outb %%al, $0x80; outb %%al, $0x21" \
			:: "a" ((unsigned long) 0xFF)); \
	cli (); \
}

/* enable all maskable interrupts */
#define enable_irqs() \
{ \
	asm volatile ("outb %%al, $0xA1; outb %%al, $0x80; outb %%al, $0x21" \
			:: "a" ((unsigned long) 0x00)); \
	sti (); \
}


/* read the value from CR0 register */
#define	read_cr0() \
({ \
	unsigned long __val; \
	asm volatile ("movl %%cr0, %0\n\t" : "=r" (__val)); \
	__val; \
})

/* write value to CR0 register */
#define write_cr0(__val) \
{ \
	asm volatile ("movl %0, %%cr0\n\t" :: "r" ((unsigned long) (__val)) \
			: "memory"); \
}


/* read the value from CR3 register */
#define	read_cr3() \
({ \
	unsigned long __val; \
	asm volatile ("movl %%cr3, %0\n\t" : "=r" (__val)); \
	__val; \
})

/* write value to CR3 register */
#define write_cr3(__val) \
{ \
	asm volatile ("movl %0, %%cr3\n\t" :: "r" ((unsigned long) (__val)) \
			: "memory"); \
}


/* read value of eflags register */
#define read_flags() \
({ \
	unsigned long __val; \
	asm volatile ("pushfl; popl %0" : "=g" (__val) : "memory", "cc"); \
	__val; \
})

/* write value to eflags register */
#define write_flags(__val) \
{ \
	asm volatile ("pushl %0; popfl" \
			:: "g" ((unsigned long) (__val)) : "memory", "cc"); \
}


/*
 * this empty asm call tell gcc not to rely on whats in its registers
 * as saved variables (this gets us around GCC optimizations) 
 * Also this flushes the prefetch queue of the processor
 */
#define invalidate() \
{ asm volatile ("\tjmp 1f\n1:\t\n" ::: "memory"); }


#endif /* __ASM_SYSTEM_H */


