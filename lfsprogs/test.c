/* $Id: test.c,v 1.12 2005/08/23 20:32:39 ppadala Exp $
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lfs.h"

#define OPENTST 0
#define RDTST 1
#define WRTST 2
#define DIRTST 3
#define SEEKRDTST 4
#define SEEKWRTST 5

int main(int argc, char *argv[])
{
	int fd = -1;
	char *buf = NULL;
	int tno, size, i, offset;
	char *fname;

	if(argc != 4) {
		printf("%s <opentst=0|rd=1|wr=2|dirtst=3|seekrd=4|seekwr=5> <file name> <number of blocks|number of files|seek addr> \n", argv[0]);
		exit(1);
	}
	/* highly overloaded names. need to change to getopt very soon */
	tno = atoi(argv[1]);
	fname = argv[2];
	size = offset = atoi(argv[3]);


	switch(tno) {
		case OPENTST:
			fd = open(fname, O_CREAT, 0600);
			if(fd < 0) {
				perror("");
				exit(1);
			}
			break;
		case RDTST:
			size = size * LFS_BSIZE;
			buf = (char *)malloc(size * sizeof(char));
			fd = open(fname, O_RDONLY);
			if(fd < 0) {
				perror("");
				exit(1);
			}
			if(read(fd, buf, size) != size) {
				perror("read error");
				exit(1);
			}
			write(1, buf, size);
			break;
		case WRTST:
			size = size * LFS_BSIZE;
			buf = (char *)malloc(size * sizeof(char));
			fd = open(fname, O_CREAT | O_WRONLY, 0600);
			if(fd < 0) {
				perror("");
				exit(1);
			}
			buf[0] = '\0';
			memset(buf, 'a', size);
			if(write(fd, buf, size) != size) {
				perror("write error");
				exit(1);
			}
			break;
		case DIRTST:
			/* size refers to the number of files to be created */
			buf = (char *)malloc(200);
			for(i = 0;i < size; ++i) {
				sprintf(buf, "%s/file%d", fname, i);
				fd = open(buf, O_CREAT, 0600);
				if(fd < 0) {
					perror("");
				}
				close(fd);
			}
			break;
		case SEEKRDTST:
			fd = open(fname, O_RDONLY);
			if(fd < 0) {
				perror("");
				exit(1);
			}
			if(lseek(fd, offset, SEEK_SET) != offset) {
				perror("");
				exit(1);
			}
			buf = (char *)malloc(100 * sizeof(char));
			read(fd, buf, 100);
			write(1, buf, 100);
			break;
		case SEEKWRTST:
			fd = open(fname, O_WRONLY);
			if(fd < 0) {
				perror("");
				exit(1);
			}
			if(lseek(fd, offset, SEEK_SET) != offset) {
				perror("");
				exit(1);
			}
			buf = (char *)malloc(100 * sizeof(char));
			memset(buf, 'a', 100);
			if(write(fd, buf, 100) != 100) {
				perror("");
				exit(1);
			}
			break;
		default:
			printf("Test no %d doesn't exist yet\n", tno);
			exit(1);
	}
	
	if(buf)
		free(buf);
	if(fd != -1)
		close(fd);
	return(0);
}
