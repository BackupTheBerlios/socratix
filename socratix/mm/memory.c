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


#include <socratix/page.h>
#include <asm/segment.h>
#include <asm/system.h>


/* The initial kernel stack */
unsigned long kstack[PAGE_SIZE >> 2];

/*
 * The description needed to load the kernel stack trough lss
 * NOTE: This is hardcoded, don't change anything here, unless
 * you know what you're doing
 */
volatile struct {
	unsigned long *addr;	/* addr of the stack start */
	unsigned short seg;	/* stack segment */
} kstack_description = {&kstack[PAGE_SIZE >> 2], KERNEL_DS};


volatile unsigned long count_ram (void)
{
	unsigned long mem_count = 0L, mem_mb = 0L;
	unsigned long cr0;

	/* store copy of CR0 */
	cr0 = read_cr0 ();

	/* plug CR0 with just PE/CD/NW
	   cache disable (486+), no-writeback (486+), 32bit mode (386+) */
	write_cr0 (cr0 | 0x00000001 | 0x40000000 | 0x20000000);

	/* search for up to 4 GB of phys. installed ram
	   in 1 MB steps */
	do {
		register unsigned long *mem, a;

		mem_mb++;
		mem_count += (1 * 1024 * 1024);
		mem = (unsigned long *) mem_count;

		a = *mem; *mem = 0x55AA55AA;

		invalidate ();

		if (*mem != 0x55AA55AA)
			mem_count = 0;
		else {
			*mem = 0xAA55AA55;

			invalidate ();

			if (*mem != 0xAA55AA55)
				mem_count = 0;
		}

		invalidate ();

		*mem = a;
	} while (mem_mb < (4 * 1024) && mem_count != 0);

	/* restore CR0 */
	write_cr0 (cr0);

	return (mem_mb << 20 /* * 1024 * 1024 */);
}



