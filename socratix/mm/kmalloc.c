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
#include <socratix/unistd.h>
#include <socratix/page.h>
#include <asm/system.h>


/* max. by kmalloc allocateable memory (in KB) */
#define MAX_KMALLOC_KB	(PAGE_SIZE >> 10)


/*
 * header of each kmalloc'ed-block, whether free or not
 */
typedef struct tagBlockHeader {
	unsigned long flags;
	union {
		struct tagBlockHeader *next;
		size_t len;
	} s;
} BlockHeader;

#define BLOCK_HEADER(p)	((BlockHeader *) (p))

/* values for the flags field */
#define BH_FREE	0x0055ffaa
#define BH_USED	0xffaa0055


/*
 * The page descriptor is at the front of every, which is in use
 * by kmalloc
 */
typedef struct tagPageDescriptor {
	struct tagPageDescriptor *next;
	BlockHeader *firstfree;
	unsigned order, nfree;
} PageDescriptor;

#define PAGE_DESC(p)	((PageDescriptor *) (PAGE_ADDR ((unsigned long) (p))))


/*
 * A size descriptor describes a specific class of malloc sizes.
 * Each class of sizes has its own free list.
 */
typedef struct tagSizeDescriptor {
	PageDescriptor *firstfree;
	size_t size;
	unsigned nblocks;

	unsigned nmallocs, nfrees, nbytesmalloced, npages;
} SizeDescriptor;

static SizeDescriptor sizes[] = {
	{NULL,   32, 127, 0, 0, 0, 0},
	{NULL,   64,  63, 0, 0, 0, 0},
	{NULL,  128,  31, 0, 0, 0, 0},
	{NULL,  252,  16, 0, 0, 0, 0},
	{NULL,  508,   8, 0, 0, 0, 0},
	{NULL, 1020,   4, 0, 0, 0, 0},
	{NULL, 2040,   2, 0, 0, 0, 0},
	{NULL, 4080,   1, 0, 0, 0, 0},
	{NULL,    0,   0, 0, 0, 0, 0}
};


#define NBLOCKS(order)		(sizes[(order)].nblocks)
#define BLOCKSIZE(order)	(sizes[(order)].size)


static inline int get_order (size_t size)
{
	int order;

	/* add the size of the header */
	size += sizeof (BlockHeader);

	for (order = 0; BLOCKSIZE (order); order++) {
		if (size <= BLOCKSIZE (order))
			return order;
	}

	return -1;
}


/* max tries, kmalloc should do to get a free page */
#define MAX_GET_FREE_PAGE_TRIES		5


void *kmalloc (size_t size, int priority)
{
	unsigned long eflags;
	int order, i, sz, tries;
	BlockHeader *p;
	PageDescriptor *page;

	if (size > (MAX_KMALLOC_KB << 10)) {
		printk ("kmalloc: allocation of %d bytes exceed kmalloc limit of %d "
			"bytes\n", size, MAX_KMALLOC_KB << 10);
		return NULL;
	}

	if ((order = get_order (size)) < 0) {
		printk ("kmalloc: too large a block (%d bytes)\n", size);
		return NULL;
	}

	eflags = read_flags ();

	for (tries = MAX_GET_FREE_PAGE_TRIES; tries--; ) {
		cli ();

		if ((page = sizes[order].firstfree) && (p = page->firstfree)) {
			if (p->flags == BH_FREE) {
				page->firstfree = p->s.next;
				page->nfree--;

				if (!page->nfree) {
					/* remove page from free list */
					sizes[order].firstfree = page->next;
					page->next = NULL;
				}

				write_flags (eflags);

				sizes[order].nmallocs++;
				sizes[order].nbytesmalloced += size;
				p->flags = BH_USED;
				p->s.len = size;

				/* increment past the header */
				return p + 1;
			}

			printk ("kmalloc: block on freelist at %08X isn't free\n",
					(long) p);
			return NULL;
		}

		write_flags (eflags);

		/* we need to get a free, new page */
		sz = BLOCKSIZE (order);

		if ((page = PAGE_DESC (__get_free_page ())) == NULL) {
			printk ("kmalloc: couldn't get a free page\n");
			return NULL;
		}

		sizes[order].npages++;

		/* loop for all but last block */
		for (i = NBLOCKS (order), p = BLOCK_HEADER (page + 1); i > 1;
				i--, p = p->s.next) {
			p->flags = BH_FREE;
			p->s.next = BLOCK_HEADER (((unsigned long) p) + sz);
		}

		/* last block */
		p->flags = BH_FREE;
		p->s.next = NULL;

		page->order = order;
		page->nfree = NBLOCKS (order);
		page->firstfree = BLOCK_HEADER (page + 1);

		cli ();

		page->next = sizes[order].firstfree;
		sizes[order].firstfree = page;
		write_flags (eflags);
	}

	panic ("kmalloc: damn ....\n");
	return NULL;
}


void kfree (void *ptr)
{
	unsigned long eflags;
	size_t size;
	int order;
	BlockHeader *p = BLOCK_HEADER (ptr) - 1;
	PageDescriptor *page, *pg2;

	page = PAGE_DESC (p);

	if ((order = page->order) < 0 ||
		(order > sizeof (sizes) / sizeof (sizes[0])) ||
		(((long) (page->next)) & ~PAGE_MASK) ||
		(p->flags != BH_USED)) {
		printk ("kfree: trying to free non-kmalloced memory: %08X\n",
				(long) ptr);
		return;
	}

	size = p->s.len;
	p->flags = BH_FREE;

	eflags = read_flags ();
	cli ();

	p->s.next = page->firstfree;
	page->firstfree = p;

	if (++page->nfree == 1) {
		if (page->next) {
			printk ("kfree: page %08X already on free list\n", page);
		} else {
			page->next = sizes[order].firstfree;
			sizes[order].firstfree = page;
		}
	}

	/* if page is completly free, free it :) */
	if (page->nfree == NBLOCKS (page->order)) {
		if (sizes[order].firstfree == page) {
			sizes[order].firstfree = page->next;
		} else {
			for (pg2 = sizes[order].firstfree; (pg2 != NULL) && (pg2->next != page); pg2 = pg2->next);

			if (pg2 != NULL)
				pg2->next = page->next;
			else {
				printk ("kfree: page %08X isn't present on free list\n",
						(long) page);
			}
		}

		free_page ((unsigned long) page);
	}

	write_flags (eflags);

	sizes[order].nfrees++;
	sizes[order].nbytesmalloced -= size;
}


