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
struct mem_list {
	struct mem_list *next;
	unsigned long id;	/* if set to 0xAA55AA55, this page is really free */
} *mem_map = NULL;

#define MEMBLK_FREE_ID	0xAA55AA55



#define enable_paging() \
{ \
	write_cr0 (read_cr0 () | 0x80000000); invalidate (); \
}

#define disable_paging() \
{ \
	write_cr0 (read_cr0 () & ~0x80000000); invalidate (); \
}



extern unsigned long count_ram (void);



void init_paging (void)
{
	register unsigned long n, d;
	unsigned long ktables;	/* number of kernel page tables */
	unsigned long kpages;


	/* determine amount of installed RAM */
	ram_size = count_ram ();


	/*
	 * calc number of pages needed by the kernel:
	 * kernel image + kernel heap + kernel page tables
	 */
	ktables = (PAGE_ALIGN (ram_size) >> (PAGE_SHIFT + 10));
	kpages = (PAGE_ALIGN (kernel_size) >> PAGE_SHIFT) + ktables;


	/*
	 * insert the links to the page tables into the kernel
	 * page directory. The page tables we're allocated
	 * directly after the kernel heap.
	 *
	 * Also this inits the page tables to map the installed
	 * phys. RAM 1:1 to linear memory, so kernel is able
	 * to access all installed phys. memory directly.
	 *
	 * NOTE: This might be a problem, if you got lot of
	 * memory, the page tables can go into the reserved
	 * area. I might fix this soon.
	 */
	for (d = 0; d < ktables; d++) {
		unsigned long *page_table, t;

		/* calc the address of the page table */
		page_table = (unsigned long *)
			(PAGE_ALIGN (kernel_size) + (d << PAGE_SHIFT));

		/* add the page table to the page directory */
		page_dir[d] = PAGE_PRESENT | PAGE_WRITE | (unsigned long) page_table;

		/* map phys. to lin. memory */
		for (t = 0; t < 1024; t++) {
			unsigned long addr;

			if (t == 0 && d == 0) {
				/*
				 * the first page is marked as not present
				 * to prevent NULL pointers within the kernel
				 */
				page_table[t] &= ~PAGE_PRESENT;
				continue;
			}

			/*
			 * check if page is physically present
			 */
			if ((addr = ((d << (PAGE_SHIFT + 10)) | (t << PAGE_SHIFT)))
					> ram_size) {
				break;
			}

			page_table[t] = PAGE_PRESENT | PAGE_WRITE | addr;
		}
	}


	/*
	 * build the mem_map list
	 * This seems to be the fastet and easiest way to manage the
	 * free memory
	 *
	 * We start after the kernel memory and ignore the reserved area
	 * between 0x9f000 - 0x100000.
	 */
	for (n = kpages; n < (PAGE_ALIGN (ram_size) >> PAGE_SHIFT); n++) {
		struct mem_list *tmp;

		if (n >= 0x9F && n < 0x100) {
			/* skip the reserved memory */
			continue;
		}

		tmp = (struct mem_list *) (n << PAGE_SHIFT);
		tmp->next = mem_map;
		tmp->id = MEMBLK_FREE_ID;
		mem_map = tmp;
	}


	write_cr3 (page_dir);
	enable_paging ();
}


unsigned long get_free_page (void)
{
	unsigned long eflags;
	struct mem_list *ptr;

	eflags = read_flags ();

	if ((ptr = mem_map) == NULL) {
		printk ("get_free_page: Not enough physical memory\n");
		write_flags (eflags);
		return 0L;
	}

	mem_map = ptr->next;

	/*
	 * chec

	/* clear the page */
	bzerol (ptr, 1024);

	return (unsigned long) ptr;
}


void free_page (unsigned long addr)
{
	unsigned long *page_table;
	unsigned long eflags;
	struct mem_list *mem;

	eflags = read_flags ();

	/* look up the page table addr in the kernel page directory */
	page_table = (unsigned long *) PAGE_ADDR (page_dir[addr >> (PAGE_SHIFT + 10)]);
	if (!page_table) {
		printk ("free_page: trying to free page %X, which is located "
			"in a not present page table\n", addr);
		write_flags (eflags);
		return;
	}

	/* check if page is present */
	if (!(page_table[(addr >> PAGE_SHIFT) & 0x3FF] & PAGE_PRESENT)) {
		printk ("free_page: trying to free not present page %X\n", addr);
		write_flags (eflags);
		return;
	}

	mem = (struct mem_list *) PAGE_ADDR (addr);

	/* check if page was not already free */
	if (mem->id == MEMBLK_FREE_ID) {
		printk ("free_page: trying to free already freed page %X\n", addr);
		write_flags (eflags);
		return;
	}

	/* add mem to free memory list */
	mem->next = mem_map;
	mem->id = MEMBLK_FREE_ID;
	mem_map = mem;

	write_flags (eflags);
}


void print_mem (void)
{
	struct mem_list *ptr;
	unsigned free = 0;

	for (ptr = mem_map; ptr != NULL; ptr = ptr->next) {
		free++;
	}

	printk ("Free memory: %d KB\n", free * 4);
}



