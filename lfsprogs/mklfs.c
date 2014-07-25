/* $Id: mklfs.c,v 1.21 2005/08/23 20:32:39 ppadala Exp $
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
/*
 * mklfs.c - Make an lfs filesystem.
 * 
 */

/*
 * Usage: mklfs [options] device
 * 
 */

#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lfs.h"

void fill_inode(struct lfs_inode *inode, int ino, int dbaddr)
{
	int i;

	inode->i_ino = ino;
	for(i = 0;i < LFS_N_BLOCKS; ++i)
		inode->i_block[i] = 0;
	if(ino == LFS_ROOT_INODE)
		inode->i_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP 
				| S_IROTH| S_IXOTH;
	else
		inode->i_mode = S_IFREG | S_IRUSR | S_IWUSR;

	if(ino == LFS_ROOT_INODE)
		inode->i_size = 3 * sizeof(lfs_dirent);
	else if(ino == LFS_IFILE_INODE)
		inode->i_size = 3 * sizeof(struct ifile_entry);
	else if(ino == LFS_SEGUSE_TABLE_INODE)
		inode->i_size = sizeof(struct cleanerinfo) + sizeof(struct segusage);
		
	inode->i_blocks = 1;
	inode->i_block[0] = dbaddr;
	inode->i_links_count = 1;

	inode->i_uid = 0; 
	inode->i_gid = 0; 
	inode->i_mtime = inode->i_atime = inode->i_ctime = time(NULL); 
	inode->i_snapi = 0;
}

void fill_ife(struct ifile_entry *ife, __u32 addr)
{
	ife->ife_version = 1;
	ife->ife_daddr = addr;
	ife->ife_atime = time(NULL);
}

void fill_segsum(struct segsum *pss)
{	
	pss->ss_magic = SS_MAGIC;
	pss->ss_create = time(NULL);
	pss->ss_nfinfo = 3; 
	pss->ss_nino  = 3;
}

void fill_finfo(struct finfo *pfi, int ino, daddr_t dbaddr)
{
	pfi->fi_nblocks = 1;
	pfi->fi_version = 1;
	pfi->fi_ino     = ino;
	pfi->fi_lbn = malloc(sizeof(daddr_t) * pfi->fi_nblocks);
	pfi->fi_blocks = malloc(sizeof(daddr_t) * pfi->fi_nblocks);
	pfi->fi_lbn[0] = 0;
	pfi->fi_blocks[0] = dbaddr;
}

void write_finfo(int fd, struct finfo *pfi)
{
	int i;
	
	write(fd, pfi, FINFOSIZE);
	for(i = 0;i < pfi->fi_nblocks; ++i)
		write(fd, &pfi->fi_lbn[i], DADDRT_SIZE);
	for(i = 0;i < pfi->fi_nblocks; ++i)
		write(fd, &pfi->fi_blocks[i], DADDRT_SIZE);
}

/* Returns partition size in bytes */
long int get_part_size(int fd)
{	int size, blksize;

	ioctl(fd, BLKSSZGET, &blksize);
	ioctl(fd, BLKGETSIZE, &size);
	return size * blksize;
}

int make_lfs(int fd)
{
	lfs_dirent dent;
	__u8 dpage[LFS_BSIZE];
	struct cleanerinfo ci;
	struct segusage seguse;
	struct lfs_super_block super;
	struct segsum segsum;
	struct ifile_entry root_ife, ifile_ife, seguse_ife;
	struct finfo root_finfo, ifile_finfo, seguse_finfo;
	struct lfs_inode root_inode, ifile_inode, seguse_inode;
	__u32 	root_iaddr, root_dbaddr, ifile_iaddr, ifile_dbaddr, 
		seguse_iaddr, seguse_dbaddr, segsum_addr;

	segsum_addr = LFS_SEGSTART;
	ifile_dbaddr = LFS_SEGSTART + 1;
	root_dbaddr = LFS_SEGSTART + 3;
	seguse_dbaddr = LFS_SEGSTART + 5;
	ifile_iaddr = ifile_dbaddr + 1;
	root_iaddr = root_dbaddr + 1;
	seguse_iaddr = seguse_dbaddr + 1;

	/* fill super block structure, we need some default values */
	memset(&super, 0, sizeof(super));
	super.s_nino = LFS_RESERVED_INODES;
	super.s_next_seg = LFS_SEGSTART;
	super.s_seg_offset = 7;
	super.s_segsize = LFS_SEGSIZE;
	super.s_version = LFS_SB_VERSION;
	super.s_magic = LFS_MAGIC;
	super.s_blocks_count = get_part_size(fd) / LFS_BSIZE;
	super.s_free_blocks_count = super.s_blocks_count - 8;
	super.s_minfreeseg = 1;
	super.s_nseg = 1;
	super.s_snapi = 0;
	super.s_ifile_iaddr[super.s_snapi] = ifile_iaddr;

	/* fill segsummary and finfos */
	fill_segsum(&segsum);
	fill_finfo(&ifile_finfo, LFS_IFILE_INODE, ifile_dbaddr);
	fill_finfo(&root_finfo, LFS_ROOT_INODE, root_dbaddr);
	fill_finfo(&seguse_finfo, LFS_SEGUSE_TABLE_INODE, seguse_dbaddr);

	/* fill inode info */
	fill_inode(&root_inode, LFS_ROOT_INODE, root_dbaddr);
	fill_inode(&ifile_inode, LFS_IFILE_INODE, ifile_dbaddr);
	fill_inode(&seguse_inode, LFS_SEGUSE_TABLE_INODE, seguse_dbaddr);

	fill_ife(&ifile_ife, ifile_iaddr);
	fill_ife(&root_ife, root_iaddr);
	fill_ife(&seguse_ife, seguse_iaddr);

	/* later gather all this in memory */
	/* we are assuming that the segment summary fits in one block */
	
	write(fd, &super, sizeof(super));

	/* go to the start of segment */
	lseek(fd, segsum_addr * LFS_BSIZE, SEEK_SET); 
	write(fd, &segsum, sizeof(segsum));
	write(fd, &ifile_iaddr, sizeof(daddr_t));
	write(fd, &root_iaddr, sizeof(daddr_t));
	write(fd, &seguse_iaddr, sizeof(daddr_t));
	write_finfo(fd, &ifile_finfo);
	write_finfo(fd, &root_finfo);
	write_finfo(fd, &seguse_finfo);

	/* ifile dblock */
	lseek(fd, ifile_dbaddr * LFS_BSIZE, SEEK_SET);
	memset(dpage, 0, sizeof(dpage));
	memcpy(dpage, &ifile_ife, sizeof(ifile_ife));
	memcpy(dpage + sizeof(ifile_ife), &root_ife, sizeof(root_ife));
	memcpy(dpage + 2 * sizeof(ifile_ife), &seguse_ife, sizeof(seguse_ife));
	write(fd, dpage, sizeof(dpage));

	/* inode blocks */
	lseek(fd, ifile_iaddr * LFS_BSIZE, SEEK_SET); 
	write(fd, &ifile_inode, sizeof(ifile_inode));

	/* root dblock */
	lseek(fd, root_dbaddr * LFS_BSIZE, SEEK_SET); 
	
	dent.inode = LFS_IFILE_INODE;
	dent.name_len = strlen(LFS_IFILE_NAME);
	strcpy(dent.name, LFS_IFILE_NAME);
	memset(dpage, 0, sizeof(dpage));
	memcpy(dpage, &dent, sizeof(lfs_dirent));
	
	dent.inode = LFS_SEGUSE_TABLE_INODE;
	dent.name_len = strlen(LFS_SEGUSE_TABLE_NAME);
	strcpy(dent.name, LFS_SEGUSE_TABLE_NAME);
	memcpy(dpage + sizeof(dent), &dent, sizeof(lfs_dirent));

	/* this should be done dynamically in future */
	dent.inode = LFS_SNAPSHOT_INODE;
	dent.name_len = strlen(LFS_SNAPSHOT_NAME);
	strcpy(dent.name, LFS_SNAPSHOT_NAME);
	memcpy(dpage + 2 * sizeof(dent), &dent, sizeof(lfs_dirent));
	write(fd, dpage, sizeof(dpage));

	lseek(fd, root_iaddr * LFS_BSIZE, SEEK_SET); 
	write(fd, &root_inode, sizeof(root_inode));

	/* seguse dblock */
	lseek(fd, seguse_dbaddr * LFS_BSIZE, SEEK_SET);
	ci.clean = super.s_free_blocks_count / LFS_SEGSIZE;
	ci.dirty = 0;
	ci.bfree = ci.avail = super.s_free_blocks_count;
	/* seguse for first segment */
	
	/* fill number of live bytes */
	seguse.su_nbytes = ifile_inode.i_size +  /* data blocks */
			   root_inode.i_size +
			   seguse_inode.i_size +
			   3 * sizeof(struct lfs_inode); /* inode blocks */

	seguse.su_olastmod = time(NULL);
	seguse.su_nsums = 1;
	seguse.su_lastmod = time(NULL);
	seguse.su_flags = SEGUSE_DIRTY;
	memset(dpage, 0, sizeof(dpage));
	memcpy(dpage, &ci, sizeof(ci));
	memcpy(dpage + sizeof(ci), &seguse, sizeof(seguse));
	write(fd, dpage, sizeof(dpage));

	lseek(fd, seguse_iaddr * LFS_BSIZE, SEEK_SET); 
	write(fd, &seguse_inode, sizeof(seguse_inode));

	return 0;
}


int main (int argc, char *argv[])
{
	char *device_name;
	int retval = 0, fd;
	
	if(argc != 2) {
		printf("Usage: %s <device name>\n", argv[0]);
		exit(1);
	}
	device_name = argv[1];

	/* Open the device */
	fd = open(device_name, O_WRONLY);

	if(fd < 0) {
		printf("Open %s failed", device_name);
		exit(1);
	}
	
	retval = make_lfs(fd);
	/* do error checking later */

	/* Close the device */
	close(fd);

	return retval < 0 ? -1: 0;
}
