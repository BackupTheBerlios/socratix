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
 * Build program, needed to build the bootsector and the kernel to
 * a disk.
 *
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* size of one sector (in bytes) */
#define	SECTOR_SIZE		(512)


/* minimal count of sectors used by the setup */
#define MIN_SETUP_SECTORS	(5)


/* maximal system size (div 16) */
#define MAX_SYSTEM_SIZE		((64 * 1024) / 16)


/*
 * Call:
 *
 * build <bootsect image> <setup image> <kernel image>
 *
 * The final image goes to stdout.
 */

int main (int argc, char *argv[])
{
	struct stat statbuf;
	unsigned long bytes, system_size;
	char buffer[2048];
	int fd, n, setup_sects;

	if (argc != 4) {
		fprintf (stderr, "usage: %s bootsect setup kernel\n", argv[0]);
		return 1;
	}

	/* try to open bootsector image */
	if ((fd = open (argv[1], O_RDONLY, 0)) < 0) {
		fprintf (stderr, "unable to open file \'%s\'\n", argv[1]);
		return 1;
	}

	/* read bootsect */
	if ((n = read (fd, buffer, sizeof (buffer))) != 512) {
		fprintf (stderr, "boot sector must be exact 512 bytes long\n");
		return 1;
	}

	/* write boot sector and close input file */
	if (write (1, buffer, 512) != 512) {
		fprintf (stderr, "unable to write bootsector\n");
		return 1;
	}

	close (fd);

	/* open setup image */
	if ((fd = open (argv[2], O_RDONLY, 0)) < 0) {
		fprintf (stderr, "unable to open file \'%s\'\n", argv[2]);
		return 1;
	}

	/* copy setup image */
	for (bytes = 0; (n = read (fd, buffer, sizeof (buffer))) > 0; bytes += n) {
		if (write (1, buffer, n) != n) {
			fprintf (stderr, "write error during setup copy\n");
			return 1;
		}
	}

	close (fd);

	/* calc the sectors used by the setup */
	setup_sects = (bytes + 0x01FF) / SECTOR_SIZE;

	if (setup_sects < MIN_SETUP_SECTORS)
		setup_sects = MIN_SETUP_SECTORS;

	fprintf (stderr, "Setup size:\t%d bytes.\n", bytes);

	/* fill the rest of the setup sectors with ZERO */
	memset (buffer, 0, sizeof (buffer));
	while (bytes < setup_sects * SECTOR_SIZE) {
		int c;

		if ((c = setup_sects * SECTOR_SIZE - bytes) > sizeof (buffer))
			c = sizeof (buffer);

		if (write (1, buffer, c) != c) {
			fprintf (stderr, "write error during sector fill\n");
			return 1;
		}

		bytes += c;
	}


	/* open kernel image */
	if ((fd = open (argv[3], O_RDONLY, 0)) < 0) {
		fprintf (stderr, "unable to open file \'%s\'\n", argv[3]);
		return 1;
	}

	/* get size of kernel image */
	if (fstat (fd, &statbuf)) {
		fprintf (stderr, "unable to stat file \'%s\'\n", argv[3]);
		return 1;
	}

	fprintf (stderr, "System size:\t%d KB.\n", (bytes = statbuf.st_size) / 1024);

	if ((system_size = (bytes + 0x0F) / 16) > MAX_SYSTEM_SIZE) {
		fprintf (stderr, "system too big. only %d KB allowed\n",
				(MAX_SYSTEM_SIZE * 16) / 1024);
		return 1;
	}

	/* write kernel */
	for ( ; bytes > 0; ) {
		int x, z;

		x = (bytes > sizeof (buffer)) ? sizeof (buffer) : bytes;

		if ((z = read (fd, buffer, x)) != x) {
			fprintf (stderr, "error while reading \'%s\'\n", argv[3]);
			return 1;
		}

		if (write (1, buffer, x) != x) {
			fprintf (stderr, "error during kernel write\n");
			return 1;
		}

		bytes -= x;
	}

	/* close kernel image file */
	close (fd);

	/* adjust values in boot sector */
	if (lseek (1, 507, SEEK_SET) != 507) {
		fprintf (stderr, "seek of final image file failed\n");
		return 1;
	}

	/* write informations to bootsector */
	buffer[0] = (unsigned char) setup_sects;
	buffer[1] = (unsigned char) (system_size & 0xFF);
	buffer[2] = (unsigned char) ((system_size >> 8) & 0xFF);
	if (write (1, buffer, 3) != 3) {
		fprintf (stderr, "write to boot sector failed\n");
		return 1;
	}

	/* ok, all fine */
	return 0;
}
	
