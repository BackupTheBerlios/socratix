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


#include <socratix/stdarg.h>
#include <socratix/ctype.h>
#include <asm/system.h>


#define ZEROPAD	(1 << 0)	/* pad with zero */
#define SIGN	(1 << 1)	/* unsigned/signed long */
#define PLUS	(1 << 2)	/* show plus */
#define SPACE	(1 << 3)	/* space if plus */
#define LEFT	(1 << 4)	/* left justified */
#define SPECIAL	(1 << 5)	/* 0x */
#define SMALL	(1 << 6)	/* use 'abcdef' instead of 'ABCDEF' */


#define do_div(n,base) ({ \
int __res; \
__asm__("divl %4":"=a" (n),"=d" (__res):"0" (n),"1" (0),"r" (base)); \
__res; })


/*
 * converts a number to a string.
 * Complaning to Linux.
 * Might be improved in the near future,
 * but for now, I need a working vsprintf
 */
static char *num2str (char * str, int num, unsigned long base, int size,
		int precision, unsigned long type)
{
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char c, sign, tmp[36];
	int i = 0;

	if (type & SMALL)
		digits = "0123456789abcdefghijklmnopqrstuvwxyz";

	if (type & LEFT)
		type &= ~ZEROPAD;

	if (base < 2 || base > 36)
		return NULL;

	c = (type & ZEROPAD) ? '0' : ' ' ;

	if (type & SIGN && num < 0) {
		sign = '-';
		num = -num;
	} else {
		sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
	}

	if (sign)
		size--;

	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}

	if (num == 0)
		tmp[i++] = '0';
	else {
	       	while (num != 0)
			tmp[i++] = digits[do_div (num, base)];
	}

	if (i > precision)
		precision = i;

	size -= precision;

	if (!(type & (ZEROPAD + LEFT))) {
		while (size-- > 0)
			*str++ = ' ';
	}

	if (sign)
		*str++ = sign;

	if (type & SPECIAL) {
		if (base == 8)
			*str++ = '0';
		else if (base == 16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}

	if (!(type & LEFT)) {
		while (size-- > 0)
			*str++ = c;
	}

	while (i < precision--) {
		*str++ = '0';
	}

	while (i-- > 0)
		*str++ = tmp[i];

	while (size-- > 0)
		*str++ = ' ';

	return str;
}


static int str2num (register const char *s, const char **endp)
{
	register int res __asm__ ("ax") = 0L;

	while (isdigit (*s))
		res = res * 10 + *s++ - '0';

	*endp = s;
	return res;
}


int vsprintf (char *buf, const char *format, va_list args)
{
	unsigned long flags;		/* flags for num2str() */
	int len, i, *ip;
	char *str = buf, *s;

	int field_width;		/* width of output field */
	int precision;			/* min. # of digits for integers; max
					   number of chars for from string */
	int qualifier;			/* 'h', 'l', or 'L' for integer fields */

	for ( ; *format ; format++) {
		if (*format != '%') {
			*str++ = *format;
			continue;
		}
			
		/* process flags */
		flags = 0L;

repeat:		switch (*++format) {
			case '-': 
				flags |= LEFT; 
				goto repeat;

			case '+':
				flags |= PLUS;
				goto repeat;

			case ' ':
				flags |= SPACE;
				goto repeat;

			case '#':
				flags |= SPECIAL;
				goto repeat;

			case '0':
				flags |= ZEROPAD;
				goto repeat;
		}
		
		/* get field width */
		field_width = -1;

		if (isdigit (*format)) {
			field_width = str2num (format, &format);
		} else if (*format == '*') {
			/* it's the next argument */
			field_width = va_arg(args, int);

			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*format == '.') {
			++format;	
			if (isdigit (*format)) {
				precision = str2num (format, &format);
			} else if (*format == '*') {
				/* it's the next argument */
				precision = va_arg(args, int);
			}

			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*format == 'h' || *format == 'l' || *format == 'L') {
			qualifier = *format++;
		}

		switch (*format) {
			/* a character */
			case 'c':
				if (!(flags & LEFT)) {
					while (--field_width > 0)
						*str++ = ' ';
				}

				*str++ = (unsigned char) va_arg (args, int);
				while (--field_width > 0)
					*str++ = ' ';
				break;

			/* a string */
			case 's':
				if (!(s = va_arg (args, char *)))
					s = "<NULL>";

				len = strlen(s);

				if (precision < 0)		precision = len;
				else if (len > precision)	len = precision;

				if (!(flags & LEFT)) {
					while (len < field_width--)
						*str++ = ' ';
				}

				for (i = 0; i < len; ++i)
					*str++ = *s++;

				while (len < field_width--)
					*str++ = ' ';
				break;

			/* an octal number */
			case 'o':
				str = num2str (str, va_arg(args, unsigned long), 8,
					field_width, precision, flags);
				break;

			/* a pointer */
			case 'p':
				if (field_width == -1) {
					field_width = 8;
					flags |= ZEROPAD;
				}

				str = num2str (str,
						(unsigned long) va_arg (args, void *),
						16, field_width, precision, flags);
				break;

			/* a hex. number */
			case 'x':
				flags |= SMALL;
			case 'X':
				str = num2str (str, va_arg (args, unsigned long), 16,
					field_width, precision, flags);
				break;

			/* a decimal digit */
			case 'd':
			case 'i':
				flags |= SIGN;
			case 'u':
				str = num2str (str, va_arg(args, unsigned long), 10,
					field_width, precision, flags);
				break;

			/* see 'man printf' */
			case 'n':
				ip = va_arg(args, int *);
				*ip = (str - buf);
				break;

			default:
				if (*format != '%')	*str++ = '%';
				if (*format)		*str++ = *format;
				else			format--;
				break;
		}
	}

	*str = '\0';
	return (int) (str - buf);
}


