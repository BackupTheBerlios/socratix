/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 *  Authors:	Volker Stroebel <mmv1@linux4us.de>
 *		Benedikt Meurer <bmeurer@fwdn.de>
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
 * I/O functions and macros
 *
 */


#ifndef __ASM_IO_H
#define __ASM_IO_H


/* output a byte value to an io port */
#define outb(val, port) \
{ \
	asm volatile (	"outb %%al, %%dx" \
			:: "a" (val), "d" (port) : "memory"); \
}


/* read a byte value from a port */
#define inb(port) \
({ \
	unsigned char val; \
	asm volatile (	"inb %%dx, %%al" \
			: "=a" (val) \
			: "d" (port) : "memory"); \
	val; \
})


/* write a byte value to a port and slow down (for timing needs) */
#define outb_p(val, port) \
{ \
	asm volatile (	"outb %%al, %%dx\n" \
			"\toutb %%al, $0x80\n" \
			:: "a" (val), "d" (port) : "memory"); \
}


/* read a byte value from a port and slow down (for timing needs) */
#define inb_p(port) \
({ \
	unsigned char val; \
	asm volatile (	"inb %%dx, %%al\n" \
			"\toutb %%al, %0x80\n" \
			: "=a" (val) \
			: "d" (port) : "memory"); \
	val; \
})


#endif /* __ASM_IO_H */


