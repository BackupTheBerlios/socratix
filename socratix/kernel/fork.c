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


#include <socratix/kmalloc.h>
#include <socratix/printk.h>
#include <socratix/page.h>
#include <socratix/task.h>
#include <asm/string.h>
#include <asm/system.h>


#define ENOMEM	12


static int copy_page_tables (Task *from, Task *to)
{
	unsigned long *pg_dir0, *pg_dir1, *pg_tab0, *pg_tab1;
	unsigned long i, t;

	if ((pg_dir1 = (unsigned long *) __get_free_page ()) == 0L)
		return 0;

	pg_dir0 = (unsigned long *) from->tss.cr3;

	for (i = 0; i < 1024; i++) {
		if ((pg_dir0[i] & PAGE_PRESENT) != PAGE_PRESENT) {
			pg_dir1[i] = pg_dir0[i];
			continue;
		}

		pg_tab0 = (unsigned long *) PAGE_ADDR (pg_dir0[i]);

		/* check for PRIVATE pages within this page table */
		for (t = 0; t < 1024; t++)
			if ((pg_tab0[t] & PAGE_PRIVATE) == PAGE_PRIVATE)
				break;

		if (t == 1024) {
			/* no private pages, we can share the hole page table */
			pg_dir1[i] = pg_dir0[i];
		} else {
			/* private pages, we need to make a new page table */
			pg_dir1[i] = __get_free_page () | PAGE_TABLE;

			pg_tab1 = (unsigned long *) PAGE_ADDR (pg_dir1[i]);

			for (t = 0; t < 1024; t++) {
				if ((pg_tab0[t] & PAGE_PRIVATE) == PAGE_PRIVATE) {
					/* private pages must be duplicated */
					pg_tab1[t] = __get_free_page () | PAGE_PRIVATE;

					bcopyl (PAGE_ADDR (pg_tab0[t]), PAGE_ADDR (pg_tab1[t]), 1024);
				} else {
					pg_tab1[t] = pg_tab0[t];
				}
			}
		}
	}

	to->tss.cr3 = (unsigned long) pg_dir1;
	return !0;
}


extern void set_tss (Task *);

extern unsigned long next_pid;

volatile int syscall_fork (IntStackFrame regs)
{
	extern void ret_from_fork (void);

	unsigned long eflags, *kernel_stack;
	IntStackFrame  *childregs;
	Task *new;

	eflags = read_flags ();

	/* allocate memory for the new process */
	if ((new = kmalloc (sizeof (Task), 0)) == NULL) {
		write_flags (eflags);
		return -ENOMEM;
	}

	/* copy the old ptask to the new one */
	*new = *current;

	if (!copy_page_tables (current, new)) {
		printk ("fork: Unable to copy page tables\n");
error:		kfree (new);
		write_flags (eflags);
		return -1;
	}

	/* get a new stack for the task */
	if ((kernel_stack = (unsigned long *) __get_free_page ()) == 0L) {
		printk ("fork: Unable to create new kernel stack\n");
		goto error;
	}

	new->pid	= next_pid++;

	new->tss.es	= KERNEL_DS;
	new->tss.cs	= KERNEL_CS;
	new->tss.ss	= KERNEL_DS;
	new->tss.ds	= KERNEL_DS;
	new->tss.fs	= KERNEL_DS;
	new->tss.gs	= KERNEL_DS;
	new->tss.esp0	= (unsigned long) &kernel_stack[PAGE_SIZE >> 2];
	new->tss.ss0	= KERNEL_DS;
	new->tss.link	= 0;
	new->tss.eflags	= regs.eflags & 0xFFFFCFFF;	/* IOPL 0 for new process */

	childregs = ((IntStackFrame *) &kernel_stack[PAGE_SIZE >> 2]) - 1;
	*childregs = regs;
	childregs->eax = 0;
	new->tss.esp = (unsigned long) childregs;
	new->tss.eip = (unsigned long) ret_from_fork;

	set_tss (new);

	/*
	 * add new task to the task list (this have to be done with
	 * interrupts disabled)
	 */
	cli ();
	new->next = current->next;
	current->next = new;

	write_flags (eflags);

	return 1;
}


