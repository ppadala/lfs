/* $Id: lfsread.c,v 1.18 2005/08/23 20:32:39 ppadala Exp $
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lfs.h"

#define SB 0
#define INO 1
#define IFILE 2
#define DB 3
#define DIRDB 4
#define SEGUSEDB 5	/* this is only the first db */
#define INDB 6
#define SEGSUM 7

void read_sb(int fd, struct lfs_super_block *sb)
{
	/* SB is always at zero block */
	lseek(fd, 0, SEEK_SET);
	if(read(fd, sb, sizeof(*sb)) != sizeof(*sb)) {
		perror("");
		exit(1);
	}	
}

int main(int argc, char *argv[])
{
	struct segsum *pss;
	struct lfs_inode inode;
	struct lfs_super_block sb;
	struct ifile_entry ife;
	int fd, block, type, i;
	char buf[LFS_BSIZE];
	char *name;

	if(argc != 4) {
		printf("Usage: %s <device name> <block number> <type(0:sb,1:ino,2:ifile db,3:db,4:dirdb,5:segusedb,6:indb,7:segsum)>\n", argv[0]);
		exit(1);
	}
	name = argv[1];
	block = atoi(argv[2]);
	type = atoi(argv[3]);
	fd = open(name, O_RDONLY);
	if(fd < 0) {
		perror("");
		exit(1);
	}

	memset(buf, 0, LFS_BSIZE);
	read_sb(fd, &sb);
	lseek(fd, block * LFS_BSIZE, SEEK_SET);
	switch(type) {
		case SB:
			printf("snapi = %d, ifile iaddr = %u\n", 
				sb.s_snapi, sb.s_ifile_iaddr[sb.s_snapi]);
			printf("nino = %u\n", sb.s_nino);
			printf("next segment = %u\n", sb.s_next_seg);
			printf("seg offset = %u\n", sb.s_seg_offset);
			printf("segsize = %u\n", sb.s_segsize);
			printf("version = %u\n", sb.s_version);
			break;
		case INO:
			if(read(fd, &inode, sizeof(inode)) != sizeof(inode)) {
				perror("");
				exit(1);
			}	
			printf("ino = %d\n", inode.i_ino);
			printf("mode = %o\n", inode.i_mode);
			printf("uid = %d\n", inode.i_uid);
			printf("size = %u\n", inode.i_size);
			printf("i_block[0] = %u\n", inode.i_block[0]);
			printf("i_block[1] = %u\n", inode.i_block[1]);
			printf("i_block[2] = %u\n", inode.i_block[2]);
			printf("i_block[3] = %u\n", inode.i_block[3]);
			printf("i_block[4] = %u\n", inode.i_block[4]);
			printf("i_block[5] = %u\n", inode.i_block[5]);
			printf("i_block[6] = %u\n", inode.i_block[6]);
			printf("i_block[7] = %u\n", inode.i_block[7]);
			printf("i_block[8] = %u\n", inode.i_block[8]);
			printf("i_block[9] = %u\n", inode.i_block[9]);
			printf("i_block[10] = %u\n", inode.i_block[10]);
			printf("i_block[11] = %u\n", inode.i_block[11]);
			printf("i_block[12] = %u\n", inode.i_block[12]);
			printf("i_block[13] = %u\n", inode.i_block[13]);
			printf("i_block[14] = %u\n", inode.i_block[14]);
			break;
		case IFILE:
			read_sb(fd, &sb);
			lseek(fd, block * LFS_BSIZE, SEEK_SET);
			for(i = 0;i < sb.s_nino && i < 64; ++i) {
				read(fd, &ife, sizeof(ife));
				printf("inode: %d, daddr: %d, version: %d\n",
				i + 1, ife.ife_daddr, ife.ife_version);
			}
			break;
		case DB:
			read(fd, buf, LFS_BSIZE);
			write(1, buf, LFS_BSIZE);
			break;
		case DIRDB:
			{
			lfs_dirent *de;
			int i;

			read(fd, buf, LFS_BSIZE);
			for(i = 0;i < LFS_BSIZE; i += LFS_DIRENT_RECLEN) {
				de = (lfs_dirent *)(buf + i);
				de->name[de->name_len] = '\0';
				printf("%d,%s\n", de->inode, de->name);
			}
			}
			break;
		case SEGUSEDB:
			{
			struct segusage *pseguse;
			int i;

			read(fd, buf, LFS_BSIZE);
			pseguse = (struct segusage *)(buf + sizeof(struct cleanerinfo));
			for(i = 0; i < sb.s_nseg; ++i) {
				printf("segment no: %d, nbytes = %d, nsums = %d %s\n", i, pseguse->su_nbytes,
				pseguse->su_nsums, ctime((time_t *)&pseguse->su_lastmod));
				++pseguse;
			}
			}
			break;
		case INDB:
			{
			int indirect_blocks = LFS_ADDR_PER_BLOCK;
			int i;
			__u32 *tempbuf[LFS_ADDR_PER_BLOCK];
		
			memset(tempbuf, 0, LFS_ADDR_PER_BLOCK * 4);
			read(fd, tempbuf, LFS_BSIZE);
			for(i = 0;i < indirect_blocks; ++i) {
				printf("%u\t", (__u32)tempbuf[i]);
				if(!((i + 1) % 5))
					printf("\n");
			}
			}
			break;
		case SEGSUM:
			{
			struct finfo fi;
			daddr_t *ino;
			int i;
			char *tempbuf;

			read(fd, buf, LFS_BSIZE);
			tempbuf = buf;
			pss = (struct segsum *)tempbuf;
			printf("nfi = %d, nino = %d, create = %s",
				pss->ss_nfinfo, pss->ss_nino,
				ctime((time_t *)&pss->ss_create));
			tempbuf += SEGSUM_SIZE;
			printf("Inode blocks: ");
			for(i = 0;i < pss->ss_nino; ++i) {
				ino = (daddr_t *)tempbuf;
				printf("%d ", *ino);
				tempbuf += DADDRT_SIZE;
			}
			printf("\n");
			for(i = 0;i < pss->ss_nfinfo; ++i) {
				int size, j;
				
				memcpy(&fi, tempbuf, FINFOSIZE);
				printf("nblocks=%d,ino=%d,", fi.fi_nblocks,
				fi.fi_ino);
				tempbuf += FINFOSIZE;
				size = fi.fi_nblocks * DADDRT_SIZE;
				fi.fi_blocks = (daddr_t *)malloc(size);
				memcpy(fi.fi_blocks, tempbuf, size);
				printf("fi_blocks[%d] = ", i);
				for(j = 0;j < fi.fi_nblocks; ++j)
					printf("%d ", fi.fi_blocks[j]);
				printf("\n");
				tempbuf += size;
			}
			}
			break;
	}
	close(fd);

	return(0);
}
