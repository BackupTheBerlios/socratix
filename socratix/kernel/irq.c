/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 *  Authors:	Benedikt Meurer <bmeurer@fwdn.de>
 *		Volker Stroebel <mmv1@linux4us.de>
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
 */


#include <asm/segment.h>
#include <asm/system.h>
#include <asm/irq.h>


/* struct for one entry in the IDT */
struct irq_struct {
	unsigned short	offset0_15;
	unsigned short	selector;
	unsigned char	unused;
	unsigned char	flags;
	unsigned short	offset16_31;
};



/* The IDT (from head.s) */
extern unsigned char idt;

#define IDT(n)	((struct irq_struct *) ((unsigned char *) &idt + (n << 3)))


/* register an irq handler for an irq */
void register_interrupt (unsigned num, void (*fn) (void))
{
	struct irq_struct *entry = IDT (num);

	entry->offset0_15	= (unsigned) fn & 0xFFFF;
	entry->selector		= KERNEL_CS;
	entry->unused		= 0x00;
	entry->flags		= 0xEE;
	entry->offset16_31	= (unsigned) fn >> 16L;
}


/* unregister the handler previously registered for the irq */
void unregister_interrupt (unsigned num)
{
	/* the dummy irq handler */
	extern void dummy_irq (void);

	register_interrupt (num, dummy_irq);
}


