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


extern unsigned long pg_dir, pg_tab0, _end;

#define page_dir	((unsigned long *) &pg_dir)
#define page_table0	((unsigned long *) &pg_tab0)

#define kernel_size	((unsigned long) &_end)


/* size of the installed ram on system */
unsigned long ram_size;


/* memory map */
unsigned long *mem_map = 0L;


#define enable_paging() \
{ \
	write_cr0 (read_cr0 () | 0x80000000); invalidate (); \
}

#define disable_paging() \
{ \
	write_cr0 (read_cr0 () & ~0x80000000); invalidate (); \
}



extern unsigned long count_ram (void);


void print_mem (void)
{
	unsigned *tmp, free = 0;

	disable_paging ();
	for (tmp = mem_map; tmp != 0; tmp = (unsigned *) *tmp) {
		free++;
	}
	enable_paging ();

	printk ("Free memory: %d KB\n", free * 4);
}


static inline unsigned long get_free_memblk (void)
{
	unsigned tmp;

	tmp = (unsigned) mem_map;
	mem_map = (unsigned long *) *mem_map;

	return tmp;
}


static inline void free_memblk (unsigned long *addr)
{
	*addr = (unsigned long) mem_map;
	mem_map = addr;
}


void free_page (unsigned long addr)
{
	unsigned long *page_table, t;

	disable_paging ();

	page_table = (unsigned long *) PAGE_ADDR (page_dir[addr >> (PAGE_SHIFT + 10)]);
	if (!page_table) {
		enable_paging ();
		printk ("free_page: trying to free a page in a not present page "
			"table\n");
		return;
	}

	t = (addr >> PAGE_SHIFT) & 0x3FF;
	if (!(page_table[t] & PAGE_PRESENT)) {
		enable_paging ();
		printk ("free_page: trying to free a not present page in page table"
			" %d at index %d\n", addr >> (PAGE_SHIFT + 10), t);
		return;
	}

	free_memblk ((unsigned long *) PAGE_ADDR (page_table[t]));
	page_table[t] = 0L;

	enable_paging ();
}


unsigned long get_free_page (void)
{
	unsigned long d, t, *page_table, eflags, mem;

	eflags = read_flags ();
	cli ();
	disable_paging ();

	for (d = 0; d < 1024; d++) {
		if ((page_dir[d] & PAGE_PRESENT) != 0) {
			page_table = (unsigned long *) PAGE_ADDR (page_dir[d]);
		} else {
			/*
			 * We need to allocate a new page table and add it
			 * to the page directory.
			 */
			if ((page_table = (unsigned long *) get_free_memblk ()) == 0) {
				enable_paging ();
				printk ("get_free_page: Not enough memory\n");
				write_flags (eflags);
				return 0L;
			}

			/* clear the new page table */
			bzerol (page_table, 1024);

			/* add it to the page directory */
			page_dir[d] = PAGE_PRESENT | PAGE_WRITE |
				(unsigned long) page_table;
		}

		/* now search for a free entry in this page table */
		for (t = 0; t < 1024; t++) {
			if ((page_table[t] & PAGE_USED) == 0) {
				if ((mem = get_free_memblk ()) == 0L) {
					enable_paging ();
					printk ("get_free_page: Not enough memory\n");
					write_flags (eflags);
					return 0L;
				}

				page_table[t] = PAGE_PRESENT | PAGE_WRITE | PAGE_USED |
						mem;

				/* clear the page */
				bzerol (mem, 1024);

				enable_paging ();
				write_flags (eflags);
				return (d << (PAGE_SHIFT + 10)) | (t << PAGE_SHIFT);
			}
		}
	}

	enable_paging ();

	printk ("Unable to fetch memory\n");
	write_flags (eflags);
	return 0L;
}


void init_paging (void)
{
	unsigned long n, kpages;


	/* determine amount of installed RAM */
	ram_size = count_ram ();


	/*
	 * page directory and first page table is already cleared (see
	 * boot/head.S), so no need to clear it here
	 */

	/* calc the amount of pages needed by the kernel image, the
	   kernel heap and the memory map */
	kpages = PAGE_ALIGN (kernel_size) >> PAGE_SHIFT;

	for (n = 1; n < kpages; n++) {
		page_table0[n] = (PAGE_PRESENT | PAGE_WRITE | PAGE_USED |
				(n << PAGE_SHIFT));
	}


	/* build the mem_map list */
	for (n = kpages; n < (PAGE_ALIGN (ram_size) >> PAGE_SHIFT); n++) {
		unsigned long *tmp;

		if (n >= 0x9F && n < 0x100) {
			/* skip the reserved memory */
			continue;
		}

		tmp = (unsigned long *) (n << PAGE_SHIFT);
		*tmp = (unsigned long) mem_map;
		mem_map = tmp;
	}


	/*
	 * the page at 0x9F000 contains the BIOS Information Block
	 * it gets mapped read only (1:1 phys:lin)
	 */
	page_table0[0x9F000 >> PAGE_SHIFT] = (PAGE_PRESENT | PAGE_USED | 0x9F000);


	/* map reserved memory (0xA0000 - 0xFFFFF) */
	for (n = (0xA0000 >> PAGE_SHIFT); n < (0x100000 >> PAGE_SHIFT); n++)
		page_table0[n] = (PAGE_PRESENT | PAGE_WRITE | PAGE_USED |
				(n << PAGE_SHIFT));


	/*
	 * page_table0[0] isn't present to prevent NULL pointers in kernel,
	 * but must be marked as used to prevent allocation of it by
	 * get_free_page
	 */
	page_table0[0] = PAGE_USED;
	page_dir[0] = PAGE_PRESENT | PAGE_WRITE | (unsigned) page_table0;


	write_cr3 (page_dir);
	enable_paging ();

	invalidate ();
}


