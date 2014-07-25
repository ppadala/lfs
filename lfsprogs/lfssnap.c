/* $Id: lfssnap.c,v 1.1 2005/08/23 20:32:15 ppadala Exp $
 *
 * Copyright (c) 2005 Pradeep Padala. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include "lfs.h"

int main (int argc, char *argv[])
{
	char *root;
	int fd;
	
	if(argc != 2) {
		printf("Usage: %s <root>\n", argv[0]);
		exit(1);
	}
	root = argv[1];

	fd = open(root, O_RDONLY);
	
	if(fd < 0) {
		printf("Open %s failed", root);
		exit(1);
	}

	if(ioctl(fd, LFS_SNAP_CREATE, NULL) < 0) 
		perror("");

	close(fd);

	return 0;
}
