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
#include <socratix/kmalloc.h>
#include <socratix/page.h>
#include <socratix/task.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/io.h>


extern unsigned long kstack[PAGE_SIZE >> 2];


Task *idle_task, *current = NULL;


struct segment_descriptor_struct
{
	unsigned short	limit_0_15;
	unsigned short	basis_0_15;
        unsigned char	basis_16_23;
        unsigned char	flags;
        unsigned char	access_limit_16_19;
        unsigned char	basis_24_31;
};
 

extern unsigned long pg_dir, gdt;

#define page_dir	((unsigned long *) &pg_dir)
#define GDT(n)		(((unsigned char *) &gdt) + n)


void set_tss (Task *tsk)
{
	struct segment_descriptor_struct *segment = (void *) GDT(tsk->tss.tr);

	segment->limit_0_15 = sizeof (tsk->tss);
	segment->basis_0_15 = ((unsigned long) &(tsk->tss)) & 0xFFFF;
	segment->basis_16_23 = (((unsigned long) &(tsk->tss)) >> 16) & 0xFF;
	segment->flags = 0x89;
	segment->access_limit_16_19 = 0x0;
	segment->basis_24_31 = (((unsigned long) &(tsk->tss)) >> 24) & 0xFF;
}


extern void do_timer (void);
extern void do_fork (void);


extern Task *current, *idle_task;


void init_sched (void)
{
	unsigned long eflags;

	eflags = read_flags ();

	if ((idle_task = kmalloc (sizeof (Task), 0)) == NULL) {
		printk ("init_sched: not enough memory\n");
		write_flags (eflags);
		return;
	}

	idle_task->next		= idle_task;
	idle_task->kernel_stack	= kstack;
	idle_task->tss.link	= 0;
	idle_task->tss.esp0	= (unsigned long) &kstack[PAGE_SIZE >> 2];
	idle_task->tss.ss0	= KERNEL_DS;
	idle_task->tss.cr3	= (unsigned long) page_dir;
	idle_task->tss.es	= KERNEL_DS;
	idle_task->tss.cs	= KERNEL_CS;
	idle_task->tss.ss	= KERNEL_DS;
	idle_task->tss.ds	= KERNEL_DS;
	idle_task->tss.fs	= KERNEL_DS;
	idle_task->tss.gs	= KERNEL_DS;
	idle_task->tss.tr	= 0x18;

	current = idle_task;
	set_tss (idle_task);

	write_tr (idle_task->tss.tr);

	cli ();
	register_interrupt (0x08, do_timer);
	register_interrupt (0x80, do_fork);

	outb(0x36,0x43);
	outb(0xa9,0x40);
	outb(0x04,0x40);

	write_flags (eflags);
}


void schedule (void)
{
	Task *next;

	next = current->next;

	if (next != current) {
/*		set_tss (next);*/

		current = next;

		/* do long jump to TSS of the new task */
		ljmp (*(((unsigned char *) &next->tss.tr) - 4));
	}
}
