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
#include <socratix/page.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/io.h>


extern unsigned long kstack[PAGE_SIZE >> 2];


struct segment_descriptor_struct
{
	unsigned short	limit_0_15;
	unsigned short	basis_0_15;
        unsigned char	basis_16_23;
        unsigned char	flags;
        unsigned char	access_limit_16_19;
        unsigned char	basis_24_31;
};
 

struct tss_struct {
	unsigned short	link, unused0;
	unsigned long	esp0;
	unsigned short	ss0, unused1;
	unsigned long	esp1;
	unsigned short	ss1, unused2;
	unsigned long	esp2;
	unsigned short	ss2, unused3;
	unsigned long	cr3;
	unsigned long	eip;
	unsigned long	eflags;
	unsigned long	eax, ecx, edx, ebx;
	unsigned long	esp, ebp, esi, edi;
	unsigned short	es, unused4;
	unsigned short	cs, unused5;
	unsigned short	ss, unused6;
	unsigned short	ds, unused7;
	unsigned short	fs, unused8;
	unsigned short	gs, unused9;
	unsigned short	ldt, unused10;
	unsigned short	debugtrap;
	unsigned short	iomapbase;
	unsigned long	tr;
};


struct task_struct {
	unsigned long *kernel_stack;	/* pointer to the task's kernel stack */
	struct tss_struct tss;
};


extern unsigned long pg_dir, gdt;

#define page_dir	((unsigned long *) &pg_dir)
#define GDT(n)		(((unsigned char *) &gdt) + n)


struct task_struct task[2] = {{0, }, {0,  }}, *current;

#define init_task	(&(task[0]))


void set_tss (struct task_struct *tsk)
{
	struct segment_descriptor_struct *segment = (void *) GDT(tsk->tss.tr);

	segment->limit_0_15 = sizeof (tsk->tss);
	segment->basis_0_15 = ((unsigned long) &(tsk->tss)) & 0xFFFF;
	segment->basis_16_23 = (((unsigned long) &(tsk->tss)) >> 16) & 0xFF;
	segment->flags = 0x89;
	segment->access_limit_16_19 = 0x0;
	segment->basis_24_31 = (((unsigned long) &(tsk->tss)) >> 24) & 0xFF;
}


void wutz (void)
{
	unsigned long i;

	for (;;) {
		printk ("B");
		sti ();
		for (i = 0; i < 0x220; i++) {
			unsigned n = i;
			while (n-- > 0);
		}
	}
}


extern unsigned long get_free_page (void);


struct pt_reg {
  long ebx;
  long ecx;
  long edx;
  long esi;
  long edi;
  long ebp;
  long eax;
  unsigned short ds, __dsu;
  unsigned short es, __esu;
  unsigned short fs, __fsu;
  unsigned short gs, __gsu;
  long eip;
  unsigned short cs, __csu;
  long eflags;
  long esp;
  unsigned short ss, __ssu;
};

extern void do_timer (void);
extern unsigned long ret_from_sched;

void init_sched (void)
{
	unsigned long eflags;
	struct pt_reg *regs;

	eflags = read_flags ();

	current = init_task;
	init_task->kernel_stack = kstack;
	init_task->tss.link = 0;
	init_task->tss.esp0 = (unsigned long) &kstack[PAGE_SIZE >> 2];
	init_task->tss.ss0 = KERNEL_DS;
	init_task->tss.cr3 = (unsigned long) page_dir;
	init_task->tss.es = KERNEL_DS;
	init_task->tss.cs = KERNEL_CS;
	init_task->tss.ss = KERNEL_DS;
	init_task->tss.ds = KERNEL_DS;
	init_task->tss.fs = KERNEL_DS;
	init_task->tss.gs = KERNEL_DS;
	init_task->tss.tr = 0x18;	/* use the temp entry in gdt */

	task[1] = task[0];
	task[1].kernel_stack = (unsigned long *) get_free_page ();
	task[1].tss.esp0 = ((unsigned long) &task[1].kernel_stack[PAGE_SIZE >> 2]) - sizeof (struct pt_reg);
	task[1].tss.esp = ((unsigned long) &task[1].kernel_stack[PAGE_SIZE >> 2]) - sizeof (struct pt_reg);
	task[1].tss.eip = (unsigned long) &ret_from_sched;
	task[1].tss.tr = 0x20;

	regs = ((struct pt_reg *) task[1].tss.esp0);
	regs->ebx = regs->ecx = regs->edx = regs->esi = regs->edi = 0;
	regs->ebp = regs->eax = 0;
	regs->ds = regs->es = regs->fs = regs->gs = KERNEL_DS;
	regs->eip = (unsigned long) wutz;
	regs->cs = KERNEL_CS;
	regs->eflags = read_flags ();
	regs->esp = task[1].tss.esp;
	regs->ss = KERNEL_DS;

	set_tss (init_task);
	set_tss (&task[1]);

	write_tr (init_task->tss.tr);

	cli ();
	register_interrupt (0x08, do_timer);

	outb(0x36,0x43);
	outb(0xa9,0x40);
	outb(0x04,0x40);

	write_flags (eflags);
}


void schedule (void)
{
	if (current == init_task)
		current = &task[1];
	else
		current = init_task;

	/* do long jump to TSS of the new task */
	ljmp (*(((unsigned char *) &current->tss.tr) - 4));
}
