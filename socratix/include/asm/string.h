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


#ifndef __ASM_STRING_H
#define __ASM_STRING_H


/* move n 4B-blocks from src to dst */
#define bcopyl(src, dst, n) \
{ \
	asm volatile (	"cld; rep; movsl" :: \
			"D" ((unsigned long *) (dst)), \
			"S" ((unsigned long *) (src)), \
			"c" ((unsigned long) (n))); \
}


/* make n 4B-blocks zero */
#define bzerol(addr, n) \
{ \
	memsetl (addr, 0L, n); \
}


/* */
#define memsetl(addr, val, n) \
{ \
	unsigned long __d0, __d1; \
	asm volatile (	"cld\n\t" \
			"rep; stosl" \
			: "=&c" (__d0), "=&D" (__d1) \
			: "1" ((unsigned long *) (addr)), "0" ((unsigned long) (n)), \
			  "a" ((unsigned long) (val)) \
			: "memory"); \
}


/* */
extern inline int strlen (register const char *s)
{
	register const char *sptr asm ("ax");

	for (sptr = s; *sptr != '\0'; sptr++)
		;

	return sptr - s;
}


#endif /* __ASM_STRING_H */

