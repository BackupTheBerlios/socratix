#
# Socratix - Main Makefile
#
#   Authors:	Benedikt Meurer <bmeurer@fwdn.de>
#		Stefan Plueger <smileos@homepage4us.de>
#		Volker Stroebel <mmv1@linux4us.de>
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
MAKE	= make
CFLAGS	= -Wall -Wstrict-prototypes -c -O2 -m486 -traditional -nostdinc -I./include 
LDFLAGS	= -Ttext 0x0000 --oformat binary -nostdlib

BOOTSECT= boot/bootsect.bin
SETUP	= boot/setup.bin
KERNEL	= boot/kernel.bin
IMAGE	= boot/socratix.bin

OBJS	= boot/head.o init/init.o kernel/kernel.o mm/mm.o

all:	boot init kernel mm tools
	$(LINKER) $(LDFLAGS) -o $(KERNEL) $(OBJS)
	tools/build $(BOOTSECT) $(SETUP) $(KERNEL) > $(IMAGE)

subdirs: dummy
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i; done

subdirsclean: dummy
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done

install: all
	dd if=$(IMAGE) of=/dev/fd0

boot:	dummy Makefile
	$(MAKE) subdirs SUBDIRS=boot

init:	dummy Makefile
	$(MAKE) subdirs SUBDIRS=init

kernel:	dummy Makefile
	$(MAKE) subdirs SUBDIRS=kernel

mm:	dummy Makefile
	$(MAKE) subdirs SUBDIRS=mm

tools:	dummy Makefile
	$(MAKE) subdirs SUBDIRS=tools

clean:	dummy
	$(MAKE) subdirsclean SUBDIRS="boot init kernel mm tools"

dummy:

