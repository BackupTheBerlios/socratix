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


#include <socratix/unistd.h>
#include <socratix/tty.h>
#include <asm/string.h>
#include <asm/io.h>


/* col, row in the screen */
static unsigned long cursor;

/* textcolor */
static unsigned char textcolor = 15;

/*
 * video mode:
 */
static enum {
	EGA_VGA		= 0,	/* 0 = EGA/VGA */
	COLOR_40x25	= 1,	/* 1 = 40x25 color */
	COLOR_80x25	= 2,	/* 2 = 80x25 color */
	MONO_80x25	= 3	/* 3 = 80x25 mono */
} video_mode;


/* pointer to video memory */
static unsigned char *video_mem;


static void tty_scroll_screen (void)
{
	if (cursor >= (25 * 160)) {
		bcopyl (video_mem + 160, video_mem, (24 * 160) / sizeof (unsigned));

		memsetl (video_mem + (24 * 160),
				0x0F200F20,	/* WHITE, ' ', WHITE, ' ' */
				160 / sizeof (unsigned));

		cursor -= 160;
	}
}


static void tty_echo (unsigned long ch)
{
	video_mem[cursor++] = (unsigned char) ch;
	video_mem[cursor++] = textcolor;

	tty_scroll_screen ();
}


void tty_init (const unsigned char *biosptr)
{
	/* get information about video mode (0x410) */
	video_mode = (biosptr[410] >> 4) & 0x03;

	if (video_mode == MONO_80x25)
		video_mem = (unsigned char *) 0xB0000;
	else /* color */
		video_mem = (unsigned char *) 0xB8000;

	/* get cursor position (0x450 = col, 0x451 = row) */
	cursor = (biosptr[0x451] * 160) + biosptr[0x450];
}


ssize_t tty_write (int fd, const char *buffer, size_t n)
{
	const char *bptr = buffer;

	for ( ; n-- > 0; bptr++) {
		switch (*bptr) {
			case '\n':	/* line feed */
				cursor += 160;
				tty_scroll_screen ();

			case '\r':	/* carriage return */
				cursor = (cursor / 160) * 160;
				break;

			case '\b':	/* backspace */
				cursor -= 2;
				break;

			case '\t':	/* tabulator */
			{
				unsigned len = 0;
				do {
					tty_echo (' ');
					len++;
				} while (len < TAB_SIZE && (cursor & TAB_MASK) != 0);
				break;
			}

			default:
				tty_echo (*bptr);
		}
	}

	/* update the cursor */
	outb (0x0F, 0x03D4);
	outb ((unsigned char) (cursor >> 1), 0x03D5);
	outb (0x0E, 0x03D4);
	outb ((unsigned char) (cursor >> 9), 0x03D5);

	/* return number of bytes written */
	return (ssize_t) (bptr - buffer);
}

