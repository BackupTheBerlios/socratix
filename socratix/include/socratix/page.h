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


#ifndef __SOCRATIX_PAGE_H
#define __SOCRATIX_PAGE_H


#include <asm/page.h>


/* align to the next page border */
#define PAGE_ALIGN(a)	((((unsigned long) (a) + (PAGE_SIZE - 1L)) \
			  >> PAGE_SHIFT) << PAGE_SHIFT)


/* get the addr of a page */
#define PAGE_MASK	0xFFFFF000
#define PAGE_ADDR(a)	((a) & PAGE_MASK)


/* */
#define PAGE_TO_PHYS(a)	\
	PAGE_ADDR (*((unsigned long *) PAGE_ADDR (page_dir[(a) >> (PAGE_SHIFT + 10)])))



#endif /* __SOCRATIX_PAGE_H */

