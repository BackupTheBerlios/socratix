#
#  Socratix - init Makefile
#
#   Authors:	Benedikt Meurer <bmeurer@fwdn.de>
#
#   Copyright (c) 2001 Socratix Development Team
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
#


CC	= gcc
LINKER	= ld
CFLAGS	= -Wall -Wstrict-prototypes -c -O2 -nostdinc -I../include
LDFLAGS	= -r -m elf_i386

OBJS	= main.o

all:	init.o

init.o:	$(OBJS) Makefile
	$(LINKER) $(LDFLAGS) -o $@ $(OBJS)

detect.o: detect.c
	$(CC) $(CFLAGS) -c -o $@ $<

main.o:	main.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o

