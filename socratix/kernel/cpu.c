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
#include <asm/system.h>


/* bits in EFLAGS */
#define EFLAGS_AC	0x40000
#define EFLAGS_ID	0x200000


/* type of installed CPU (3 = i386, 4 = 486, 5 = 586, ...) */
unsigned long x86 = 3;


void init_cpu (void)
{
	unsigned long eflags;

	eflags = read_flags ();

	/* flip AC bit in EFLAGS */
	write_flags (eflags | EFLAGS_AC);

	/* check if AC bit was modified */
	if (((read_flags () ^ eflags) & EFLAGS_AC) != 0) {
		/* we got at least a 486 */
		x86 = 4;

		/*
		 * check ID flag. If we're on a straight 486DX or 486SX
		 * we can't change the ID flag
		 */
		write_flags (eflags ^ EFLAGS_ID);

		if ((read_flags () & EFLAGS_ID) != 0) {
			/*
			 * we got a cpu > 486 SX/DX, so we can use the CPUID instr.
			 * to get more information about the CPU
			 */
			unsigned long cpu_type;

			/* call CPUID 1, to get the cpu type */
			asm volatile ("cpuid" : "=a" (cpu_type) : "a" (1L));

			x86 = (cpu_type & 0xF00) >> 8;
		}
	}

	/* set CR0 corresponding to cpu installed */
	if (x86 > 3) {
		/* 486 or better: save PG, PE, ET and set AM, WP, NE, MP */
		write_cr0 ((read_cr0 () & 0x80000011) | 0x50022);
	} else {
		/* 386: save PG, PE, ET and set MP */
		write_cr0 ((read_cr0 () & 0x80000011) | 0x02);
	}

	/* output information to user */
	printk ("CPU #0: i%d86\n", x86);

	/* restore original EFLAGS */
	write_flags (eflags);
}

