#
#  Socratix - kernel Makefile
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
CFLAGS	= -Wall -Wstrict-prototypes -c -O2 -m486 -nostdinc -I../include -fno-builtin
LDFLAGS	= -m elf_i386 -r

OBJS	= cpu.o irq.o panic.o printk.o sched.o tty.o vsprintf.o

all:	kernel.o

kernel.o: $(OBJS) Makefile
	$(LINKER) $(LDFLAGS) -o kernel.o $(OBJS)

cpu.o: cpu.c
	$(CC) $(CFLAGS) -c $< -o $@

irq.o: irq.c
	$(CC) $(CFLAGS) -c $< -o $@

panic.o: panic.c
	$(CC) $(CFLAGS) -c $< -o $@

printk.o: printk.c
	$(CC) $(CFLAGS) -c $< -o $@

sched.o: sched.c
	$(CC) $(CFLAGS) -c $< -o $@

tty.o: tty.c
	$(CC) $(CFLAGS) -c $< -o $@

vsprintf.o: vsprintf.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o

