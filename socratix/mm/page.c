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


extern unsigned long pg_dir, _end;

#define page_dir	((unsigned long *) &pg_dir)
#define kernel_size	((unsigned long) &_end)


/* size of the installed ram on system */
unsigned long ram_size;


/* memory map */
struct mem_blk {
	struct mem_blk *next;
	unsigned long state;	/* if set to MEMBLK_FREE, this page is really free (only for debugging, will be removed, when mm tests are completed :)) */
} *mem_map = NULL;

#define MEMBLK_FREE	0xAA55AA55



extern unsigned long count_ram (void);


extern unsigned long kstack[PAGE_SIZE >> 2];

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
		page_dir[d] = PAGE_TABLE | (unsigned long) page_table;

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

			if (addr == (unsigned) &kstack[0]) {
				/* stack is private */
				page_table[t] = PAGE_PRIVATE | addr;
			} else {
				/* we can share the page */
				page_table[t] = PAGE_SHARED | addr;
			}
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
		struct mem_blk *tmp;

		if (n >= 0x9F && n < 0x100) {
			/* skip the reserved memory */
			continue;
		}

		tmp = (struct mem_blk *) (n << PAGE_SHIFT);
		tmp->next = mem_map;
		tmp->state = MEMBLK_FREE;
		mem_map = tmp;
	}


	/* Ok, that was hard work, now we can hopefully enable paging */
	write_cr3 (page_dir);
	write_cr0 (read_cr0 () | 0x80000000);
	invalidate ();
}


unsigned long __get_free_page (void)
{
	unsigned long eflags;
	struct mem_blk *ptr;

	eflags = read_flags ();
	cli ();

	if ((ptr = mem_map) != NULL) {
		mem_map = ptr->next;

		/* check if page is really free */
		if (ptr->state == MEMBLK_FREE) {
			ptr->state = 0;
			write_flags (eflags);

			return (unsigned long) ptr;
		}

		/*
		 * mem block is on free list, but isn't marked as
		 * free. This could mean, that someone messed up with
		 * mm or that your installed system RAM is damaged.
		 */
		panic ("get_free_page: Got an entry from the free mem block list, "
			"which isn't really free. This means, that someone messed "
			"up with memory management or that your installed system "
			"RAM is broken. In each case we couldn't continue well, "
			"so we'll give up!");

	}

	/* no phys. mem left */
	printk ("get_free_page: Not enough physical memory\n");
	write_flags (eflags);
	return 0L;
}


unsigned long get_free_page (void)
{
	unsigned long addr;

	if ((addr = __get_free_page ()) != 0L) {
		/* clear the page */
		bzerol (addr, PAGE_SIZE >> 2);
	}

	return addr;
}


void free_page (unsigned long addr)
{
	unsigned long *page_table;
	unsigned long eflags;
	struct mem_blk *ptr;

	eflags = read_flags ();
	cli ();

	/*
	 * lookup the page table addr in the kernel page directory and
	 * check if page is present
	 */
	if ((page_table = (unsigned long *) PAGE_ADDR (page_dir[addr >> (PAGE_SHIFT + 10)])) != NULL
	&& (page_table[(addr >> PAGE_SHIFT) & 0x3FF] & PAGE_PRESENT) != 0) {
		ptr = (struct mem_blk *) PAGE_ADDR (addr);

		/* check if page wasn't already freed */
		if (ptr->state != MEMBLK_FREE) {
			ptr->next = mem_map;
			ptr->state = MEMBLK_FREE;
			mem_map = ptr;

end:			write_flags (eflags);
			return;
		}

		/* page was already free, some messed up in mm */
		printk ("free_page: trying to free already free page %08X\n", ptr);
		goto end;
	}

	/* we're trying to free a not present page */
	printk ("free_page: trying to free not present page %08X\n", PAGE_ADDR (addr));
	goto end;
}


void print_mem (void)
{
	struct mem_blk *ptr;
	unsigned free = 0;

	for (ptr = mem_map; ptr != NULL; ptr = ptr->next) {
		free++;
	}

	printk ("Free memory: %d KB\n", free * 4);
}



