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


#ifndef __SOCRATIX_CTYPE_H
#define __SOCRATIX_CTYPE_H


#define	isalnum(c)	(isalpha (c) || isdigit (c))
#define	isalpha(c)	(isupper (c) || islower (c))
#define	isdigit(c)	((c) >= '0' && (c) <= '9')
#define	islower(c)	((c) >= 'a' && (c) <= 'z')
#define	isspace(c)	((c) == ' ' || (c) == '\f' || (c) == '\n' || \
			 (c) == '\r' || (c) == '\t' || (c) == '\v')
#define	isupper(c)	((c) >= 'A' && (c) <= 'Z')
#define	isxdigit(c)	(isdigit (c) || ((c) >= 'a' && (c) <= 'f') || \
					((c) >= 'A' && (c) <= 'F'))


#define	tolower(c)	(isupper (c) ? (c) - ('A' - 'a') : (c))
#define	toupper(c)	(islower (c) ? (c) - ('a' - 'A') : (c))


#endif /* __SOCRATIX_CTYPE_H */

