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
 */


#include <socratix/printk.h>
#include <socratix/tty.h>
#include <asm/system.h>


extern void init_paging (void);
extern void init_idt (void);


extern unsigned long get_free_page (void);
extern void free_page (unsigned long);
extern void print_mem (void);


void start_kernel (void)
{
	unsigned int i;
	unsigned long *mem;

	tty_init (0x9F000);

	init_paging ();

	enable_irqs ();

	if ((mem = (unsigned long *) get_free_page ()) != 0L) {
		printk ("get memory at %X\n", mem);
		free_page (mem);
	}

	print_mem ();
#if 0
	print_mem ();

	cli ();
	mem = (unsigned long *) get_free_page ();
	sti ();

	printk ("got a page at %X\n", mem);

	cli (); free_page (mem); sti ();
	cli (); free_page (mem); sti ();

	mem[10] = 0;

	print_mem ();
#endif
	for (;;) {hlt ();}
}
