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


