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


#ifndef __ASM_PAGE_H
#define __ASM_PAGE_H


/* page size and page shift */
#define	PAGE_SIZE	(4 * 1024)	/* 4K */
#define	PAGE_SHIFT	(12L)


/* page flags */
#define PAGE_PRESENT	(1 << 0)
#define PAGE_WRITE	(1 << 1)
#define PAGE_USER	(1 << 2)
#define PAGE_ACCESSED	(1 << 5)
#define PAGE_DIRTY	(1 << 6)


#endif /* __ASM_PAGE_H */

