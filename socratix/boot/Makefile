#
#  Socratix - boot Makefile
#
#   Authors:	Benedikt Meurer <bmeurer@fwdn.de>
#		Stefan Plueger <smileos@homepage4us.de>
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
CPP	= gcc -E -I../include # cpp
LINKER	= ld
CFLAGS	= -c -traditional -nostdinc -O2 -I../include
LDFLAGS	= -Ttext 0x0 -s --oformat binary -nostdlib

all:	bootsect.bin setup.bin head.o

bootsect.s: bootsect.S Makefile
	$(CPP) -traditional $< -o $@

bootsect.o: bootsect.s
	$(CC) $(CFLAGS) -o $@ $<

bootsect.bin: bootsect.o
	$(LINKER) $(LDFLAGS) $< -o $@

setup.s: setup.S bootsect.bin Makefile
	$(CPP) -traditional $< -o $@

setup.o: setup.s
	$(CC) $(CFLAGS) -o $@ $<

setup.bin: setup.o
	$(LINKER) $(LDFLAGS) $< -o $@

head.s: head.S Makefile
	$(CPP) -traditional $< -o $@

head.o: head.s
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o *.s *.bin

