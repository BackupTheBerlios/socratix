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


#ifndef __SOCRATIX_TASK_H
#define __SOCRATIX_TASK_H


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


typedef struct tagTask {
	unsigned long *kernel_stack;	/* pointer to the task's kernel stack */
	struct tss_struct tss;
	struct tagTask *next;
} Task;


/*
 * After an interrupt occurs, the stack look like this
 * (NOTE: reverse order, ss gets pushed first!)
 */
struct reg_struct {
	unsigned long	ebx;
	unsigned long	ecx;
	unsigned long	edx;
	unsigned long	esi;
	unsigned long	edi;
	unsigned long	ebp;
	unsigned long	eax;
	unsigned short	ds, __unused0;
	unsigned short	es, __unused1;
	unsigned long	eip;
	unsigned short	cs, __unused2;
	unsigned long	eflags;
	unsigned long	esp;
	unsigned short	ss, __unused3;
};


extern Task *idle_task, *current;


#endif /* __SOCRATIX_TASK_H */

