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


#include <socratix/unistd.h>


/*
 * the mem* functions in the normal case (byte operations) and in dword
 * manner (the "l" suffix), where all operations work on dwords
 */

/* set n bytes in mem to v */
extern inline void *memset (void *mem, unsigned char v, size_t n)
{
	unsigned long __d0, __d1;
	asm volatile (	"cld\n\t"
			"rep; stosb"
			: "=&c" (__d0), "=&D" (__d1)
			: "1" ((unsigned long) (mem)), "0" (n), "a" (v)
			: "memory");
	return mem;
}

/* set n dwords in mem to v */
extern inline void *memsetl (void *mem, unsigned long v, size_t n)
{
	unsigned long __d0, __d1;
	asm volatile (	"cld\n\t"
			"rep; stosl"
			: "=&c" (__d0), "=&D" (__d1)
			: "1" ((unsigned long) (mem)), "0" (n), "a" (v)
			: "memory");
	return mem;
}


extern inline void *memmove (void *dest, const void *src, size_t n)
{
	unsigned long __d0, __d1, __d2;

	if (dest < src) {
		asm volatile (	"cld\n\t"
				"rep; movsb"
				: "=&c" (__d0), "=&S" (__d1), "=&D" (__d2)
				:"0" (n), "1" ((unsigned long) src),
				"2" ((unsigned long) dest)
				: "memory");
	} else { /* src <= dest */
		asm volatile (	"std\n\t"
				"rep; movsb; cld"
				: "=&c" (__d0), "=&S" (__d1), "=&D" (__d2)
				:"0" (n), "1" (n - 1 + (const unsigned char *) src),
				"2" (n - 1 + (unsigned char *) dest)
				:"memory");
	}

	return dest;
}


extern inline void *memmovel (void *dest, const void *src, size_t n)
{
	unsigned long __d0, __d1, __d2;

	if (dest < src) {
		asm volatile (	"cld\n\t"
				"rep; movsl"
				: "=&c" (__d0), "=&S" (__d1), "=&D" (__d2)
				:"0" (n), "1" ((unsigned long) src),
				"2" ((unsigned long) dest)
				: "memory");
	} else { /* src <= dest */
		asm volatile (	"std\n\t"
				"rep; movsl; cld"
				: "=&c" (__d0), "=&S" (__d1), "=&D" (__d2)
				:"0" (n), "1" (n - 1 + (const unsigned long *) src),
				"2" (n - 1 + (unsigned long *) dest)
				:"memory");
	}

	return dest;
}


/*
 * The BSD mem functions are macros to the "real" mem* functions
 * Did I mentioned, I love BSD? :)
 */
#define bzero(mem, n)		memset ((mem), 0, (n))
#define bzerol(mem, n)		memsetl ((mem), 0L, (n))

#define bcopy(src, dest, n)	memmove ((dest), (src), (n))
#define bcopyl(src, dest, n)	memmovel ((dest), (src), (n))


/*
 * The string functions
 */

/* */
extern inline size_t strlen (const char *s)
{
	register const char *sptr asm ("ax");

	for (sptr = s; *sptr != '\0'; sptr++)
		;

	return sptr - s;
}


#endif /* __ASM_STRING_H */

