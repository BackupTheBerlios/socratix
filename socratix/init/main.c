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
#include <socratix/task.h>
#include <socratix/tty.h>
#include <asm/system.h>

#include <socratix/kmalloc.h>


extern void init_sched (void);

extern void init_paging (void);
extern void init_idt (void);


extern unsigned long __get_free_page (void);
extern unsigned long get_free_page (void);
extern void free_page (unsigned long);
extern void print_mem (void);


extern void init_cpu (void);


int fork (void)
{
	int res asm ("ax");
	asm volatile ("int $0x80" : "=a" (res));
	return res;
}


void start_kernel (void)
{
	int i;
	Task *p;

	init_paging ();

	tty_init (0x9F000);

	init_cpu ();

	print_mem ();

	init_sched ();

	enable_irqs ();

	if ((i = fork ()) == 0) {
		/* child */
		for (;;) {
			for (i = 0; i < 0x200; i++) {
				unsigned n = i;
				while (n-- > 0);
			}
			printk ("task[%d]\n", current->pid);
		}
	} else if (i < 0) {
		/* error */
		printk ("error during fork!\n");

	}

	/* father */

	/* we fork again :) */
	if ((i = fork ()) == 0) {
		/* next child */
		for (;;) {
			printk ("task[%d]\n", current->pid);
			for (i = 0; i < 0x200; i++) {
				unsigned n = i;
				while (n-- > 0);
			}
		}
	} else if (i < 0) {
		printk ("error during fork\n");
	}

	/* ... and again :) */
	if ((i = fork ()) == 0) {
		/* next child */
		for (;;) {
			for (i = 0; i < 0x100; i++) {
				unsigned n = i;
				while (n-- > 0);
			}
			printk ("task[%d]\n", current->pid);
			for (i = 0; i < 0x100; i++) {
				unsigned n = i;
				while (n-- > 0);
			}
		}
	} else if (i < 0) {
		printk ("error during fork\n");
	}

	for (;;) {
		printk ("task[%d]\n", current->pid);
		for (i = 0; i < 0x200; i++) {
			unsigned n = i;
			while (n-- > 0);
		}
	}

	for (;;) {}
}

