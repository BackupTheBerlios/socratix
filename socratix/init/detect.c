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
 * Functions for detecting installed Hardware before initializing the
 * kernel (paging, tty, ...) itself.
 *
 */


#include <asm/system.h>


/* from head.S */
extern unsigned char x86;


volatile void detect_cpu (void)
{
	unsigned long eflags;

	eflags = read_flags ();
	write_flags (eflags | 0x40000);	/* flip AC bit in EFLAGS */

	/* check if AC was modified */
	if (((read_flags () ^ eflags) & 0x40000) != 0) {
		x86 = 4;	/* at least 486 */

		/* check ID flag, if we're on a straight 486DX, SX
		   we cannot change the ID flag */
		write_flags (eflags ^ 0x200000);

		if ((read_flags () & 0x200000) != 0) {
			/*
			 * we got a cpu > 486 DX/SX
			 */

			unsigned long cpu_type;
			
			/* call CPUID 1, to get the cpu type */
			asm volatile ("cpuid" : "=a" (cpu_type) : "a" (1L));

			x86 = (cpu_type & 0x00000F00) >> 8;
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

	/* restore original EFLAGS */
	write_flags (eflags);
}



