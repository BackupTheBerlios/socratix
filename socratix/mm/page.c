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
#include <asm/string.h>
#include <asm/system.h>


extern unsigned long pg_dir, pg_tab0, pg_tab1023, _end;

#define page_dir	((unsigned long *) &pg_dir)
#define page_table0	((unsigned long *) &pg_tab0)
#define page_table1023	((unsigned long *) &pg_tab1023)


#define kernel_size	((unsigned long) &_end)


/*
 * contains the linear addresses of the page tables.
 * This is the simplest solution for the problem, that
 * else, all page table addresses in the page directory
 * have to be identically (lin. : phys = 1:1). The other
 * alternativ would be to disable paging, every time, we
 * access the page dir, but that would be terrible.
 */
unsigned long *page_tables_lin[1024];


/* size of the installed ram on system */
unsigned long ram_size;


/* memory map */
unsigned char *mem_map;
unsigned long mem_map_sz;


static volatile unsigned long count_ram (void)
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


void print_mem (void)
{
	unsigned int i;
	unsigned int mem_used = 0L, mem_free = 0L;

	for (i = 0; i < mem_map_sz; i++) {
		if ( mem_map[i] == 0)
			mem_free++;
		else
			mem_used++;
	}

	printk ("print_mem: %d KB used, %d KB free\n", mem_used * 4, mem_free * 4);
}


unsigned long get_free_page (void)
{
	register unsigned long phys, d, t;

search_mem:
	for (phys = 0; phys < mem_map_sz; phys++)
		if (mem_map[phys] == 0)
			break;

	if (phys == mem_map_sz) {
		/* no free phys. mem */
		printk ("get_free_page: No physical memory available\n");
		return 0L;
	}

	/*
	 * search for free page
	 * NOTE: the 1024th page table is reserved. It contains the
	 * linear addresses of the page tables, allocated at runtime
	 * Cause else, we would have to disable paging, every time,
	 * we access the page directory (see description of page_tables_lin
	 * for more information)
	 */
	for (d = 0; d < 1023; d++) {
		unsigned long *page_table;

		if ((page_dir[d] & PAGE_PRESENT) == 0) {
			/*
			 * the page dir entry isn't present, we need to
			 * allocate a new page table and add it to the
			 * page directory.
			 */


			/*
			 * we also need to make this page linear available
			 * therefor we take the next free entry in the 1024th
			 * page table
			 */
			page_table = page_table1023;
			for (t = 0; t < 1024; t++) {
				if ((page_table[t] & PAGE_USED) == 0) {
					page_table[t] = (PAGE_PRESENT | PAGE_USED |
							PAGE_WRITE |
							(phys << PAGE_SHIFT));
					break;
				}
			}

			if (t >= 1024) {
				printk ("get_free_page: Not enough lin. mem\n");
				return 0L;
			}


			/*
			 * add the page table to the page directory
			 */
 			page_dir[d] = page_table[t];
			mem_map[phys] = 1;


			/*
			 * remember linear address of the newly allocated
			 * page table
			 */
			page_tables_lin[d] = (unsigned long *)
				((1023 << (PAGE_SHIFT + 10)) | (t << PAGE_SHIFT));


			/*
			 * clear page table 
			 */
			bzerol (page_tables_lin[d], 1024);

			invalidate ();

			goto search_mem;
		}

		page_table = page_tables_lin[d];

		for (t = 0; t < 1024; t++) {
			if ((page_table[t] & PAGE_USED) == 0) {
				unsigned long addr;

				/* mark the phys. mem block as used */
				mem_map[phys] = 1;

				/* OK, we got a free page table entry, assign it */
				page_table[t] = (PAGE_PRESENT | PAGE_USED | PAGE_WRITE
						| (phys << PAGE_SHIFT));

				addr = (d << (PAGE_SHIFT + 10)) | (t << PAGE_SHIFT);

				/* clear the page */
				bzerol (addr, 1024);

				return addr;
			}
		}
	}

	/* no free page :-/ */
	return 0L;
}


void init_paging (void)
{
	unsigned n, kpages;

	/* determine size of installed RAM */
	ram_size = count_ram ();


	/* clear the page directory and the first page table */
	bzerol (page_dir, 1024);
	bzerol (page_tables_lin, 1024);
	bzerol (page_table0, 1024);
	bzerol (page_table1023, 1024);


	/* init the memory map (behind the kernel heap) */
	mem_map_sz = ram_size >> PAGE_SHIFT;
	mem_map = (unsigned char *) PAGE_ALIGN (kernel_size);

	/* clear the memory map */
	bzerol (mem_map, mem_map_sz / sizeof (long));


	/* calc the amount of pages needed by the kernel image, the
	   kernel heap and the memory map */
	kpages = (PAGE_ALIGN (kernel_size) >> PAGE_SHIFT) +
		(PAGE_ALIGN (mem_map_sz) >> PAGE_SHIFT);

	for (n = 1; n < kpages; n++) {
		page_table0[n] = (PAGE_PRESENT | PAGE_WRITE | PAGE_USED |
				(n << PAGE_SHIFT));
		mem_map[n] = 1;
	}


	/*
	 * the page at 0x9F000 contains the BIOS Information Block
	 * it gets mapped, read only
	 */
	page_table0[0x9F000 >> PAGE_SHIFT] = (PAGE_PRESENT | PAGE_USED | 0x9F000);
	mem_map[0x9F000 >> PAGE_SHIFT] = 1;


	/* map reserved memory (0xA0000 - 0xFFFFF) */
	for (n = (0xA0000 >> PAGE_SHIFT); n < (0x100000 >> PAGE_SHIFT); n++) {
		page_table0[n] = (PAGE_PRESENT | PAGE_WRITE | PAGE_USED |
				(n << PAGE_SHIFT));
		mem_map[n] = 1;
	}

	/*
	 * page_table0[0] isn't present to prevent NULL pointers in kernel,
	 * but must be marked as used to prevent allocation of it by
	 * get_free_page
	 */
	page_table0[0] = PAGE_USED;

	page_dir[0] = PAGE_PRESENT | PAGE_WRITE | PAGE_USED | (unsigned) page_table0;
	page_tables_lin[0] = page_table0;


	/*
	 * The last page table (1024th) is reserved for the addresses of
	 * the page tables, that will be allocated later
	 */
	page_dir[1023] = PAGE_PRESENT | PAGE_WRITE | PAGE_USED |
		(unsigned) page_table1023;
	page_tables_lin[1023] = page_table1023;


	write_cr3 (page_dir);
	write_cr0 (read_cr0 () | 0x80000000);

	invalidate ();
}


